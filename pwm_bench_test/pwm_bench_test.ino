#include <Arduino.h>
#include <pwm.h>

// Standalone bench example for Arduino UNO R4 WiFi.
// Lift the wheels before powering the ESC. Disconnect power to end a test.
// These values come from the course EXAMPLE, not calibration of your car.
// Neutral is not a verified brake command. No automatic ESC calibration is run.
constexpr uint32_t kEscPeriodUs = 10000;     // 100 Hz
constexpr uint32_t kSteeringPeriodUs = 20000; // 50 Hz
constexpr int kEscNeutralUs = 1530;
int steeringNeutralUs = 1522; // Replace with the measured center to keep it after reset.
constexpr int kThrottleStepUs = 10;
constexpr int kThrottleLimitUs = 150;
int throttleOffsetUs = 0;
constexpr int kSteeringTestOffsetUs = 50;
constexpr int kSteeringLimitUs = 400; // Software cap, not a measured mechanical limit.
constexpr uint32_t kTestDurationMs = 2000;

PwmOut esc(D6);
PwmOut steering(D5);
bool outputsReady = false;
bool testActive = false;
bool commandFault = false;
uint32_t testStartedMs = 0;
char throttleCommand[8];
uint8_t throttleCommandLength = 0;
bool receivingThrottle = false;
char targetChannel = 't';
int steeringOffsetUs = 0;

void setNeutral() {
  // Try both outputs even if one write fails.
  const bool driveOk = esc.pulseWidth_us(kEscNeutralUs);
  const bool steerOk = steering.pulseWidth_us(steeringNeutralUs);
  if (!driveOk || !steerOk) commandFault = true;
  testActive = false;
  throttleOffsetUs = 0;
  steeringOffsetUs = 0;
  Serial.print("Steering center (us): ");
  Serial.println(steeringNeutralUs);
}

void setup() {
  Serial.begin(115200); // Do not wait for a USB serial connection.
  const bool driveOk = esc.begin(kEscPeriodUs, uint32_t(kEscNeutralUs));
  const bool steerOk = steering.begin(kSteeringPeriodUs, uint32_t(steeringNeutralUs));
  outputsReady = driveOk && steerOk;
  if (!outputsReady) {
    // Leave any successfully initialized channel at its neutral pulse.
    Serial.println("PWM initialization failed. Disconnect ESC power.");
    return;
  }
  Serial.println("Ready at neutral. Send ? for commands.");
}

void loop() {
  if (!outputsReady) return;
  const uint32_t now = millis();
  if (testActive && uint32_t(now - testStartedMs) >= kTestDurationMs) {
    setNeutral();
    Serial.println("Test ended: neutral requested.");
  }
  // Process at most one byte per loop. No delay() or blocking input parsing.
  if (!Serial.available()) return;
  char command = char(Serial.read());
  int requestedOffsetUs = 0;
  // Absolute offsets: t120\n for throttle or s25\n for steering.
  if (command == 'n' || command == 'x') {
    receivingThrottle = false;
  } else if (receivingThrottle) {
    if (command == '\r') return;
    if (command != '\n') {
      if (throttleCommandLength < sizeof(throttleCommand) - 1 &&
          ((command >= '0' && command <= '9') ||
           (command == '-' && throttleCommandLength == 0))) {
        throttleCommand[throttleCommandLength++] = command;
      } else {
        receivingThrottle = false;
        setNeutral();
        Serial.println("Invalid throttle command: neutral requested.");
      }
      return;
    }
    receivingThrottle = false;
    throttleCommand[throttleCommandLength] = '\0';
    char* end = nullptr;
    const long value = strtol(throttleCommand, &end, 10);
    const int limit = targetChannel == 's' ? kSteeringLimitUs : kThrottleLimitUs;
    const long minimum = targetChannel == 'c' ? 1422 : -limit;
    const long maximum = targetChannel == 'c' ? 1622 : limit;
    if (end == throttleCommand || *end != '\0' ||
        value < minimum || value > maximum) {
      setNeutral();
      Serial.println("Invalid throttle value: neutral requested.");
      return;
    }
    requestedOffsetUs = int(value);
    command = targetChannel;
  } else if (command == 't' || command == 's' || command == 'c') {
    targetChannel = command;
    receivingThrottle = true;
    throttleCommandLength = 0;
    return;
  }
  if (command == '\r' || command == '\n') return;
  if (command == '?') {
    Serial.println("f: throttle +10 us; b: throttle -10 us; limit: +/-150 us");
    Serial.println("l/r: steering test; n or x: neutral on both channels");
    Serial.println("t<number>: absolute throttle offset in us, -150 to 150");
    Serial.println("s<number>: absolute steering offset in us, -400 to 400; resets throttle");
    Serial.println("c<number>: steering center in us, 1422 to 1622; session only");
    Serial.println("Neutral after 2 seconds without a control command.");
    return;
  }
  if (command == 'n' || command == 'x') {
    setNeutral();
    Serial.println("Neutral requested. Send a new command for another test.");
    return;
  }
  if (commandFault) {
    Serial.println("PWM write fault. Disconnect power and inspect the setup.");
    return;
  }
  bool ok = true;
  switch (command) {
    case 'c':
      steeringNeutralUs = requestedOffsetUs;
      setNeutral();
      Serial.println("Neutral requested with updated steering center.");
      return;
    case 't':
      steeringOffsetUs = 0;
      ok = steering.pulseWidth_us(steeringNeutralUs);
      throttleOffsetUs = requestedOffsetUs;
      ok = esc.pulseWidth_us(kEscNeutralUs - throttleOffsetUs) && ok;
      break;
    case 's':
      throttleOffsetUs = 0;
      ok = esc.pulseWidth_us(kEscNeutralUs);
      steeringOffsetUs = requestedOffsetUs;
      ok = steering.pulseWidth_us(steeringNeutralUs + steeringOffsetUs) && ok;
      break;
    // The course example assigns SMALLER pulses to forward.
    // Actual direction and deadband depend on the ESC setup.
    case 'f':
    case 'b':
      throttleOffsetUs = constrain(throttleOffsetUs +
          (command == 'f' ? kThrottleStepUs : -kThrottleStepUs),
          -kThrottleLimitUs, kThrottleLimitUs);
      ok = esc.pulseWidth_us(kEscNeutralUs - throttleOffsetUs);
      break;
    case 'l':
      setNeutral();
      steeringOffsetUs = -kSteeringTestOffsetUs;
      ok = !commandFault && steering.pulseWidth_us(steeringNeutralUs - kSteeringTestOffsetUs);
      break;
    case 'r':
      setNeutral();
      steeringOffsetUs = kSteeringTestOffsetUs;
      ok = !commandFault && steering.pulseWidth_us(steeringNeutralUs + kSteeringTestOffsetUs);
      break;
    default: return;
  }
  if (!ok) {
    commandFault = true;
    setNeutral();
    Serial.println("PWM write failed. Disconnect ESC power.");
    return;
  }
  testStartedMs = now;
  testActive = true;
  Serial.print("Throttle offset (us): ");
  Serial.print(throttleOffsetUs);
  Serial.print(" | ESC pulse (us): ");
  Serial.println(kEscNeutralUs - throttleOffsetUs);
  Serial.print("Steering offset (us): ");
  Serial.print(steeringOffsetUs);
  Serial.print(" | Servo pulse (us): ");
  Serial.println(steeringNeutralUs + steeringOffsetUs);
}
