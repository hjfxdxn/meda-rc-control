// UNO R4 WiFi autonomous tug-of-war. One pull per physical reset.
#include <Arduino.h>
#include <pwm.h>
#include <Arduino_LED_Matrix.h>
#include <math.h>

constexpr int CAR_NUMBER = 82;
static_assert(CAR_NUMBER == 81 || CAR_NUMBER == 82, "Select the actual chassis");
constexpr uint32_t NEUTRAL_US = CAR_NUMBER == 82 ? 1530 : 1510; // Verify on the ESC.
constexpr int START_US = CAR_NUMBER == 82 ? 1330 : 1240; // Trial launch pulse beyond measured onset.
constexpr int PULL_US = CAR_NUMBER == 81 ? 1200 : 1100; // Trial strong forward command; not a calibrated endpoint.
constexpr uint32_t RAMP_US = 300000;
static_assert(PULL_US >= 1000 && PULL_US <= START_US && START_US < NEUTRAL_US, "Invalid forward ramp");
constexpr uint32_t CENTER_US = CAR_NUMBER == 82 ? 1550 : 1449;
constexpr uint8_t TRIG = 8, ECHO = 12; // Front HC-SR04 on J7.
constexpr float STOP_CM = 20.0f; // Trial distance; measure stopping margin under load.
constexpr uint32_t RUN_US = 10000000, PING_INTERVAL_US = 60000, ECHO_TIMEOUT_US = 25000;
PwmOut esc(D6), steering(D5);
constexpr float SOUND_RMS_THRESHOLD = CAR_NUMBER == 81 ? 50.0f : 10.0f; // 12-bit ADC counts, AC RMS.
enum class Phase { Settling, Clear, Armed, Pulling, Stopped };
Phase phase = Phase::Settling;
unsigned clearWindows = 0, matchWindows = 0;
uint32_t settledAt, startedAt, lastPing;
int lastDriveUs = START_US;
bool escReady = false;

// The left seven columns show the chassis. The right four show its state.
ArduinoLEDMatrix matrix;
bool matrixReady = false;
constexpr int DISPLAY_CAR = CAR_NUMBER;
constexpr uint8_t DIGIT_EIGHT[] = {7, 5, 7, 5, 7};
constexpr uint8_t DIGIT_ONE[] = {2, 6, 2, 2, 7};
constexpr uint8_t DIGIT_TWO[] = {7, 1, 7, 4, 7};
constexpr uint8_t WAIT_MARK[] = {0, 0, 15, 0, 0};
constexpr uint8_t READY_MARK[] = {0, 1, 2, 10, 4};
constexpr uint8_t DRIVE_MARK[] = {4, 2, 1, 2, 4};
constexpr uint8_t STOP_MARK[] = {0, 9, 6, 6, 9};

void showStatus(const uint8_t* mark) {
  if (!matrixReady) return;
  uint8_t pixels[8][12] = {};
  const uint8_t* unit = DISPLAY_CAR == 81 ? DIGIT_ONE : DIGIT_TWO;
  for (int row = 0; row < 5; ++row) {
    for (int col = 0; col < 3; ++col) {
      pixels[row + 1][col] = (DIGIT_EIGHT[row] >> (2 - col)) & 1;
      pixels[row + 1][col + 4] = (unit[row] >> (2 - col)) & 1;
    }
    for (int col = 0; col < 4; ++col) {
      pixels[row + 1][col + 8] = (mark[row] >> (3 - col)) & 1;
    }
  }
  matrix.renderBitmap(pixels, 8, 12);
}

void stopPull(const char* reason) {
  phase = Phase::Stopped;
  bool neutralOK = escReady && esc.pulseWidth_us(NEUTRAL_US);
  digitalWrite(LED_BUILTIN, LOW);
  showStatus(STOP_MARK);
  if (Serial) {
    Serial.print("STOP "); Serial.println(reason);
    if (!neutralOK) Serial.println("PWM fault: disconnect ESC power");
  }
}

bool rangeClear() {
  // A missing or invalid echo is unknown, not a near obstacle.
  if (digitalRead(ECHO) == HIGH) return true;
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long width = pulseIn(ECHO, HIGH, ECHO_TIMEOUT_US);
  float cm = width * 0.01715f;
  return width == 0 || cm >= STOP_CM; // Stop only for a measured distance below 20 cm.
}

void listenForWhistle() {
  // Measure AC RMS over approximately 50 ms. No frequency or jitter gate.
  int64_t sum = 0;
  uint64_t sumSquares = 0;
  uint32_t due = micros();
  for (unsigned i = 0; i < 500; ++i) {
    while (int32_t(micros() - due) < 0) {}
    int32_t value = int32_t(analogRead(A0)) - 2048;
    sum += value;
    sumSquares += int64_t(value) * value;
    due += 100;
  }
  double mean = double(sum) / 500.0;
  double variance = double(sumSquares) / 500.0 - mean * mean;
  float rms = sqrt(variance > 0 ? variance : 0);
  bool match = rms >= SOUND_RMS_THRESHOLD;
  static uint32_t lastReport = 0;
  if (Serial && uint32_t(millis() - lastReport) >= 250) {
    lastReport = millis();
    Serial.print("RMS:"); Serial.print(rms, 2);
    Serial.print(" State:"); Serial.println(phase == Phase::Clear ? "WAIT_QUIET" : "ARMED");
  }
  if (phase == Phase::Clear) {
    clearWindows = !match ? clearWindows + 1 : 0;
    if (clearWindows >= 3) {
      phase = Phase::Armed;
      digitalWrite(LED_BUILTIN, HIGH);
      showStatus(READY_MARK);
      if (Serial) Serial.println(CAR_NUMBER == 81 ? "ARMED: waiting for sound (RMS >= 50)" : "ARMED: waiting for sound (RMS >= 10)");
    }
    return;
  }
  matchWindows = match ? matchWindows + 1 : 0;
  if (matchWindows < 2) return;
  if (!rangeClear()) { stopPull("front obstacle below 20 cm at start"); return; }
  if (Serial.available()) return; // Process pending operator input before applying power.
  startedAt = micros();
  if (!esc.pulseWidth_us(START_US)) { stopPull("ESC output fault"); return; }
  lastDriveUs = START_US;
  lastPing = startedAt;
  phase = Phase::Pulling;
  showStatus(DRIVE_MARK);
  if (Serial) Serial.println(CAR_NUMBER == 81 ? "START: forward command issued; ramp to 1200 us" : "START: forward command issued; ramp to 1100 us");
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT); digitalWrite(TRIG, LOW);
  pinMode(ECHO, INPUT); pinMode(LED_BUILTIN, OUTPUT);
  analogReadResolution(12);
  escReady = esc.begin(10000, NEUTRAL_US);
  bool steeringReady = steering.begin(20000, CENTER_US);
  matrixReady = matrix.begin(); // Allocate the display timer after both PWM channels.
  showStatus(WAIT_MARK);
  if (!escReady || !steeringReady) { stopPull("PWM initialization"); return; }
  settledAt = millis();
}

void loop() {
  if (phase == Phase::Stopped) return;
  if (phase == Phase::Pulling && uint32_t(micros()-startedAt) >= RUN_US) {
    stopPull("10 second limit"); return;
  }
  for (int i=0; i<8 && Serial.available(); ++i) {
    int c = Serial.read();
    if (c=='x' || c=='X' || c=='n' || c=='N') { stopPull("operator"); return; }
  }
  if (phase == Phase::Settling) {
    if (uint32_t(millis()-settledAt) >= 3000) phase = Phase::Clear;
    return;
  }
  if (phase != Phase::Pulling) { listenForWhistle(); return; }
  uint32_t now = micros();
  // Non-blocking ramp: stop checks above always take priority.
  uint32_t elapsed = uint32_t(now - startedAt);
  int requested = elapsed >= RAMP_US ? PULL_US
      : START_US - int(uint32_t(START_US - PULL_US) * elapsed / RAMP_US);
  if (requested != lastDriveUs) {
    if (!esc.pulseWidth_us(requested)) { stopPull("ESC ramp output fault"); return; }
    lastDriveUs = requested;
  }
  // Keep the final deadline free of blocking echo acquisition.
  if (uint32_t(now-startedAt) >= RUN_US-ECHO_TIMEOUT_US-2000) return;
  if (uint32_t(now-lastPing) < PING_INTERVAL_US) return;
  lastPing = now;
  if (!rangeClear()) stopPull("front obstacle below 20 cm");
}
