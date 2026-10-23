#include <Arduino.h>
#include <pwm.h>
#include "wifi_bench.h"

// Standalone bench example for Arduino UNO R4 WiFi.
// Lift the wheels before powering the ESC. Disconnect power to end a test.
// These values come from the course EXAMPLE, not calibration of your car.
// Neutral is not a verified brake command. No automatic ESC calibration is run.
constexpr uint32_t kEscPeriodUs = 10000;     // 100 Hz
constexpr uint32_t kSteeringPeriodUs = 20000; // 50 Hz
int escNeutralUs = 1530;
int steeringLeftUs = 1472, steeringRightUs = 1572; // Narrow uncalibrated startup range.
constexpr uint32_t kEscSettleMs = 3000;
uint32_t escNeutralSinceMs = 0;
int steeringNeutralUs = 1522; // Replace with the measured center to keep it after reset.
constexpr int kThrottleStepUs = 10;
constexpr int kThrottleLimitUs = 600;
int throttleOffsetUs = 0;
constexpr int kSteeringTestOffsetUs = 50;
constexpr int kSteeringLimitUs = 800; // Software cap, not a measured mechanical limit.
constexpr uint32_t kTestDurationMs = 2000;

PwmOut esc(D6);
PwmOut steering(D5);
bool outputsReady = false;
bool testActive = false;
bool commandFault = false;
uint32_t testStartedMs = 0;
char throttleCommand[32];
uint8_t throttleCommandLength = 0;
bool receivingThrottle = false;
char targetChannel = 't';
int steeringOffsetUs = 0;

void reportConfiguration() {
  benchLog.print("CAL2 ESC:"); benchLog.print(escNeutralUs);
  benchLog.print(" Center:"); benchLog.print(steeringNeutralUs);
  benchLog.print(" Left:"); benchLog.print(steeringLeftUs);
  benchLog.print(" Right:"); benchLog.println(steeringRightUs);
}

void setNeutral() {
  // Try both outputs even if one write fails.
  const bool driveOk = esc.pulseWidth_us(escNeutralUs);
  const bool steerOk = steering.pulseWidth_us(steeringNeutralUs);
  if (!driveOk || !steerOk) commandFault = true;
  testActive = false;
  throttleOffsetUs = 0;
  steeringOffsetUs = 0;
  reportConfiguration();
}

void setup() {
  Serial.begin(115200); // Do not wait for a USB serial connection.
  const bool driveOk = esc.begin(kEscPeriodUs, uint32_t(escNeutralUs));
  const bool steerOk = steering.begin(kSteeringPeriodUs, uint32_t(steeringNeutralUs));
  outputsReady = driveOk && steerOk;
  if (!outputsReady) {
    // Leave any successfully initialized channel at its neutral pulse.
    benchLog.println("PWM initialization failed. Disconnect ESC power.");
    return;
  }
  escNeutralSinceMs = millis();
  benchLog.println("Ready at neutral. Send ? for commands.");
  reportConfiguration();
  startBenchWiFi();
}

void processCommandByte(char command) {
  const uint32_t now = millis();
  int requestedOffsetUs = 0;
  // Absolute offsets: t120\n for throttle or s25\n for steering.
  if (command == 'n' || command == 'x') {
    receivingThrottle = false;
  } else if (receivingThrottle) {
    if (command == '\r') return;
    if (command != '\n') {
      if (throttleCommandLength < sizeof(throttleCommand) - 1 &&
          ((command >= '0' && command <= '9') ||
           (command == '-' && throttleCommandLength == 0) ||
           (command == ',' && targetChannel == 'g'))) {
        throttleCommand[throttleCommandLength++] = command;
      } else {
        receivingThrottle = false;
        setNeutral();
        benchLog.println("Invalid throttle command: neutral requested.");
      }
      return;
    }
    receivingThrottle = false;
    throttleCommand[throttleCommandLength] = '\0';
    if (targetChannel == 'g') {
      int center, left, right; char extra;
      if (commandFault || sscanf(throttleCommand, "%4d,%4d,%4d%c", &center, &left, &right, &extra) != 3 ||
          left < 900 || right > 2100 || left >= center || center >= right) {
        setNeutral(); benchLog.println("Invalid steering profile: neutral requested."); return;
      }
      steeringNeutralUs = center; steeringLeftUs = left; steeringRightUs = right;
      setNeutral(); benchLog.println("Neutral requested with updated steering profile."); return;
    }
    char* end = nullptr;
    const long value = strtol(throttleCommand, &end, 10);
    const int limit = targetChannel == 's' ? kSteeringLimitUs : kThrottleLimitUs;
    const long minimum = targetChannel == 'c' ? steeringLeftUs : targetChannel == 'e' ? 1000 : -limit;
    const long maximum = targetChannel == 'c' ? steeringRightUs : targetChannel == 'e' ? 2000 : limit;
    if (end == throttleCommand || *end != '\0' ||
        value < minimum || value > maximum) {
      setNeutral();
      benchLog.println("Invalid throttle value: neutral requested.");
      return;
    }
    requestedOffsetUs = int(value);
    command = targetChannel;
  } else if (command == 't' || command == 's' || command == 'c' || command == 'e' || command == 'g') {
    targetChannel = command;
    receivingThrottle = true;
    throttleCommandLength = 0;
    return;
  }
  if (command == '\r' || command == '\n') return;
  if (command == '?') {
    benchLog.println("f: throttle +10 us; b: throttle -10 us; offset cap: +/-600 us; absolute ESC output: 1000..2000 us");
    benchLog.println("l/r: steering test; n or x: neutral on both channels");
    benchLog.println("t<number>: absolute throttle offset in us, -600 to 600");
    benchLog.println("s<number>: absolute steering offset in us, -800 to 800, clamped to steering profile; resets throttle");
    benchLog.println("c<number>: steering center in us, within current steering limits; session only");
    benchLog.println("e<number>: ESC neutral, 1000..2000 us; wait 3 seconds after changing");
    benchLog.println("g<center>,<left>,<right>: steering profile, 900..2100 us");
    benchLog.println("Neutral after 2 seconds without a control command.");
    reportConfiguration();
    return;
  }
  if (command == 'n' || command == 'x') {
    setNeutral();
    benchLog.println("Neutral requested. Send a new command for another test.");
    return;
  }
  if (commandFault) {
    benchLog.println("PWM write fault. Disconnect power and inspect the setup.");
    return;
  }
  if ((command == 't' || command == 'f' || command == 'b') &&
      uint32_t(now - escNeutralSinceMs) < kEscSettleMs) {
    setNeutral(); benchLog.println("ESC settling: neutral requested."); return;
  }
  bool ok = true;
  switch (command) {
    case 'e':
      escNeutralUs = requestedOffsetUs; escNeutralSinceMs = now;
      setNeutral(); benchLog.println("Neutral requested with updated ESC neutral."); return;
    case 'c':
      steeringNeutralUs = requestedOffsetUs;
      setNeutral();
      benchLog.println("Neutral requested with updated steering center.");
      return;
    case 't':
      steeringOffsetUs = 0;
      ok = steering.pulseWidth_us(steeringNeutralUs);
      throttleOffsetUs = requestedOffsetUs;
      throttleOffsetUs = escNeutralUs - constrain(escNeutralUs - throttleOffsetUs, 1000, 2000);
      ok = esc.pulseWidth_us(escNeutralUs - throttleOffsetUs) && ok;
      break;
    case 's':
      throttleOffsetUs = 0;
      ok = esc.pulseWidth_us(escNeutralUs);
      steeringOffsetUs = constrain(steeringNeutralUs + requestedOffsetUs, steeringLeftUs, steeringRightUs) - steeringNeutralUs;
      ok = steering.pulseWidth_us(steeringNeutralUs + steeringOffsetUs) && ok;
      break;
    // The course example assigns SMALLER pulses to forward.
    // Actual direction and deadband depend on the ESC setup.
    case 'f':
    case 'b':
      steeringOffsetUs = 0;
      ok = steering.pulseWidth_us(steeringNeutralUs);
      throttleOffsetUs = constrain(throttleOffsetUs +
          (command == 'f' ? kThrottleStepUs : -kThrottleStepUs),
          -kThrottleLimitUs, kThrottleLimitUs);
      throttleOffsetUs = escNeutralUs - constrain(escNeutralUs - throttleOffsetUs, 1000, 2000);
      ok = esc.pulseWidth_us(escNeutralUs - throttleOffsetUs) && ok;
      break;
    case 'l':
      setNeutral();
      steeringOffsetUs = max(-kSteeringTestOffsetUs, steeringLeftUs - steeringNeutralUs);
      ok = !commandFault && steering.pulseWidth_us(steeringNeutralUs + steeringOffsetUs);
      break;
    case 'r':
      setNeutral();
      steeringOffsetUs = min(kSteeringTestOffsetUs, steeringRightUs - steeringNeutralUs);
      ok = !commandFault && steering.pulseWidth_us(steeringNeutralUs + steeringOffsetUs);
      break;
    default: return;
  }
  if (!ok) {
    commandFault = true;
    setNeutral();
    benchLog.println("PWM write failed. Disconnect ESC power.");
    return;
  }
  testStartedMs = now;
  testActive = true;
  benchLog.print("Throttle offset (us): ");
  benchLog.print(throttleOffsetUs);
  benchLog.print(" | ESC pulse (us): ");
  benchLog.println(escNeutralUs - throttleOffsetUs);
  benchLog.print("Steering offset (us): ");
  benchLog.print(steeringOffsetUs);
  benchLog.print(" | Servo pulse (us): ");
  benchLog.println(steeringNeutralUs + steeringOffsetUs);
}

void loop() {
  if (!outputsReady) return;
  const uint32_t timeout = remoteMotion ? 500 : kTestDurationMs;
  if (testActive && uint32_t(millis() - testStartedMs) >= timeout) {
    setNeutral();
    if (remoteMotion) revokeRemote();
    benchLog.println("Test ended: neutral requested.");
  }
  if (Serial.available()) {
    const char c = char(Serial.read());
    // USB STOP always wins. Other USB commands cannot mix with a Wi-Fi lease.
    if (c == 'n' || c == 'x') { revokeRemote(); remoteMotion = false; processCommandByte(c); }
    else if (!remoteToken) { remoteMotion = false; processCommandByte(c); }
  }
  pollBenchWiFi();
}
