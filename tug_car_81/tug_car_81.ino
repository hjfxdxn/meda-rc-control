// UNO R4 WiFi autonomous tug-of-war. One pull per physical reset.
#include <Arduino.h>
#include <pwm.h>
#include <Arduino_LED_Matrix.h>
#include "ra0020_classifier.h"

// Car 81: Tamiya TEU-105BK. Verify neutral and loaded starting torque on this chassis.
constexpr uint32_t NEUTRAL_US = 1510;
constexpr int PULL_US = 1340;
constexpr uint32_t CENTER_US = 1449;
constexpr uint8_t TRIG = 8, ECHO = 12; // Front HC-SR04 on J7.
constexpr float STOP_CM = 20.0f; // Trial distance; measure stopping margin under load.
constexpr uint32_t RUN_US = 10000000, PING_INTERVAL_US = 60000, ECHO_TIMEOUT_US = 25000;
PwmOut esc(D6), steering(D5);
WhistleClassifier detector;
uint16_t samples[AUDIO_SAMPLES];
enum class Phase { Settling, Clear, Armed, Pulling, Stopped };
Phase phase = Phase::Settling;
unsigned clearWindows = 0, matchWindows = 0;
uint32_t settledAt, startedAt, lastPing;
bool escReady = false;

// The left seven columns show the chassis. The right four show its state.
ArduinoLEDMatrix matrix;
bool matrixReady = false;
constexpr int DISPLAY_CAR = 81;
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
  if (digitalRead(ECHO) == HIGH) return false;
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long width = pulseIn(ECHO, HIGH, ECHO_TIMEOUT_US);
  float cm = width * 0.01715f;
  return cm > STOP_CM && cm <= 400.0f;
}

// Optional backup: build with this flag enabled AND select D4 to GND before
// the bout. The operator holds RESET until the whistle. A selected cold boot
// also runs this sequence, so power up with D4 open and wheels lifted.
constexpr bool ENABLE_RESET_BACKUP = false;
constexpr uint8_t BACKUP_SELECT = 4;
bool manualSelected = false;

void startPull() {
  if (!rangeClear()) { stopPull("front obstacle or missing echo at start"); return; }
  if (Serial.available()) return;
  startedAt = micros();
  if (!esc.pulseWidth_us(PULL_US)) { stopPull("ESC output fault"); return; }
  lastPing = startedAt;
  phase = Phase::Pulling;
  showStatus(DRIVE_MARK);
}

void listenForWhistle() {
  uint32_t due = micros();
  unsigned lateSamples = 0;
  bool timingOK = true;
  for (int i = 0; i < AUDIO_SAMPLES; ++i) {
    while (int32_t(micros() - due) < 0) {}
    uint32_t late = micros() - due;
    if (late > 25) ++lateSamples;
    if (late >= 100) timingOK = false;
    samples[i] = analogRead(A0);
    due += 100;
  }
  auto sound = detector.analyze(samples);
  bool valid = timingOK && lateSamples <= 2 && !sound.clipped;
  bool match = sound.rms >= MIN_RMS_COUNTS && sound.toneRatio >= MIN_TONE_RATIO;
  if (phase == Phase::Clear) {
    clearWindows = valid && !match ? clearWindows + 1 : 0;
    if (clearWindows >= 3) {
      phase = Phase::Armed;
      digitalWrite(LED_BUILTIN, HIGH);
      showStatus(READY_MARK);
      if (Serial) Serial.println("ARMED: waiting for whistle A or B");
    }
    return;
  }
  matchWindows = valid && match ? matchWindows + 1 : 0;
  if (matchWindows < 2) return;
  startPull();
}

void setup() {
  Serial.begin(115200);
  pinMode(BACKUP_SELECT, INPUT_PULLUP);
  manualSelected = ENABLE_RESET_BACKUP && digitalRead(BACKUP_SELECT) == LOW;
  pinMode(TRIG, OUTPUT); digitalWrite(TRIG, LOW);
  pinMode(ECHO, INPUT); pinMode(LED_BUILTIN, OUTPUT);
  analogReadResolution(12);
  escReady = esc.begin(10000, NEUTRAL_US);
  bool steeringReady = steering.begin(20000, CENTER_US);
  matrixReady = matrix.begin(); // Allocate the display timer after both PWM channels.
  showStatus(WAIT_MARK);
  if (!escReady || !steeringReady) { stopPull("PWM initialization"); return; }
  detector.begin();
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
    if (uint32_t(millis()-settledAt) < 3000) return;
    if (manualSelected) startPull();
    else phase = Phase::Clear;
    return;
  }
  if (phase != Phase::Pulling) { listenForWhistle(); return; }
  uint32_t now = micros();
  // Keep the final deadline free of blocking echo acquisition.
  if (uint32_t(now-startedAt) >= RUN_US-ECHO_TIMEOUT_US-2000) return;
  if (uint32_t(now-lastPing) < PING_INTERVAL_US) return;
  lastPing = now;
  if (!rangeClear()) stopPull("front obstacle or missing echo");
}
