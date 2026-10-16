// UNO R4 WiFi: one-shot sound-triggered ESC bench test.
// LIFT THE DRIVEN WHEELS before powering the ESC. Neutral is not verified braking.
// ESC signal D6. No steering servo output. All devices must share ground.
// Pulse values follow pwm_bench_test; verify direction and neutral on your vehicle.
// Microphone: AO=A0, DO=D7, 5V-compatible VCC=5V, GND=GND.
// HC-SR04 default GPIO mode: Trig=D9, Echo=D11, VCC=5V, GND=GND.
// Classification uses AO waveform; DO and its threshold do not decide A/B.
#include <Arduino.h>
#include <pwm.h>
#include "ra0020_classifier.h"

constexpr uint32_t ESC_PERIOD_US = 10000; // 100 Hz, as in pwm_bench_test
constexpr int ESC_NEUTRAL_US = 1530;
constexpr int START_ESC_PULSE_US = 1580; // User-tested reverse pulse.
constexpr uint32_t MOTOR_TEST_MS = 2000;
constexpr uint32_t ESC_SETTLE_MS = 3000; // Minimum wait, not proof of ESC arming.
static_assert(START_ESC_PULSE_US >= ESC_NEUTRAL_US - 150 &&
              START_ESC_PULSE_US <= ESC_NEUTRAL_US + 150,
              "Start pulse must remain within 150 us of neutral");
PwmOut esc(D6);
bool outputsReady = false, motorRunning = false, testFinished = false;
bool pwmFault = false, cancelled = false;
uint32_t neutralSinceMs = 0, motorStartedMs = 0;

void stopMotor() {
  // Request neutral; never restart after a fault.
  bool driveOK = esc.pulseWidth_us(ESC_NEUTRAL_US);
  if (!driveOK) pwmFault = true;
  motorRunning = false;
  testFinished = true;
}

bool serviceStopCommand() {
  // Bounded parsing. Only stop commands are accepted; no serial command starts it.
  for (int i = 0; i < 8 && Serial.available(); ++i) {
    char c = char(Serial.read());
    if (c == 'x' || c == 'n') {
      stopMotor(); cancelled = true;
      Serial.println("Motor:STOPPED Reason:USER_STOP Reset_required:1");
      return true;
    }
  }
  return false;
}

void startMotor() {
  if (!outputsReady || pwmFault || cancelled || motorRunning || testFinished) return;
  // Timestamp before applying throttle; avoid extending the run during output calls.
  motorStartedMs = millis();
  bool driveOK = esc.pulseWidth_us(START_ESC_PULSE_US);
  if (!driveOK) { pwmFault = true; stopMotor(); return; }
  motorRunning = true;
}

const unsigned long RANGE_INTERVAL_MS = 250;
const unsigned long ECHO_TIMEOUT_US = 40000;
const unsigned long SAMPLE_PERIOD_US = 100; // 10 kHz
uint16_t audioSamples[AUDIO_SAMPLES];
WhistleClassifier classifier;
unsigned long lastRangeMs = 0;
float distanceCm = -1;
char previousCandidate = '?';
unsigned int consecutiveMatches = 0;
// Require three quiet/non-target blocks after reset before arming.
// START is latched until reset. Only a detected sound can start the motor test.
unsigned int clearBlocks = 0, startMatches = 0;
bool armed = false, startDetected = false;

float readDistanceCm() {
  digitalWrite(9, LOW); delayMicroseconds(2);
  digitalWrite(9, HIGH); delayMicroseconds(12);
  digitalWrite(9, LOW);
  unsigned long echoUs = pulseIn(11, HIGH, ECHO_TIMEOUT_US);
  return echoUs ? echoUs * 0.0343f / 2.0f : -1.0f;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(7, INPUT);
  pinMode(9, OUTPUT); digitalWrite(9, LOW);
  pinMode(11, INPUT);
  analogReadResolution(12);
  classifier.begin();
  Serial.begin(115200);
  bool driveOK = esc.begin(ESC_PERIOD_US, uint32_t(ESC_NEUTRAL_US));
  outputsReady = driveOK;
  if (!outputsReady) {
    pwmFault = true;
    // Reassert neutral only on channels that initialized successfully.
    if (driveOK) esc.pulseWidth_us(ESC_NEUTRAL_US);
    Serial.println("Motor:FAULT Reason:PWM_INIT Disconnect_ESC_power:1");
    return;
  }
  neutralSinceMs = millis();
  Serial.println("Motor:NEUTRAL State:ESC_SETTLE Keep_wheels_lifted:1");
}

void loop() {
  if (!outputsReady) return;
  // Deadline has priority over sampling, ultrasound, and serial reporting.
  if (motorRunning && uint32_t(millis()-motorStartedMs) >= MOTOR_TEST_MS) {
    stopMotor();
    Serial.println(pwmFault ? "Motor:FAULT Disconnect_ESC_power:1" :
                             "Motor:STOPPED Reason:TIMEOUT Reset_required:1");
  }
  if (serviceStopCommand()) return;
  // No sample blocks, pulseIn, or telemetry while running; service timeout rapidly.
  if (motorRunning || testFinished || pwmFault || cancelled) return;
  if (uint32_t(millis()-neutralSinceMs) < ESC_SETTLE_MS) return;
  // Do not run ultrasonic measurements or Serial writes inside this sample block.
  unsigned long due = micros();
  unsigned long firstSampleUs = 0, lastSampleUs = 0, maxLateUs = 0;
  unsigned int lateSamples = 0;
  for (int i = 0; i < AUDIO_SAMPLES; ++i) {
    while ((int32_t)(micros()-due) < 0) {}
    unsigned long actual = micros();
    unsigned long lateUs = actual-due;
    if (lateUs > 25) ++lateSamples;
    if (lateUs > maxLateUs) maxLateUs = lateUs;
    if (i == 0) firstSampleUs = actual;
    lastSampleUs = actual;
    audioSamples[i] = analogRead(A0);
    due += SAMPLE_PERIOD_US;
  }
  float sampleHz = (AUDIO_SAMPLES-1)*1000000.0f/(lastSampleUs-firstSampleUs);
  // Allow at most two small scheduling delays, but reject a missed sample period.
  bool timingOK = lateSamples <= 2 && maxLateUs < SAMPLE_PERIOD_US &&
                  sampleHz >= 9900.0f && sampleHz <= 10100.0f;
  SoundResult r = classifier.analyze(audioSamples);
  // A/B share a carrier near 3.2 kHz. Modulation classification is diagnostic only.
  bool toneMatch = timingOK && !r.clipped && r.rms >= MIN_RMS_COUNTS &&
                   r.toneRatio >= MIN_TONE_RATIO;
  const char *reason = !timingOK ? "TIMING" : r.clipped ? "CLIPPING" :
                       r.rms < MIN_RMS_COUNTS ? "WEAK" :
                       r.toneRatio < MIN_TONE_RATIO ? "NOT_TARGET" : "MATCH";
  if (!armed) {
    if (timingOK && !r.clipped && !toneMatch) {
      if (++clearBlocks >= 3) armed = true;
    } else clearBlocks = 0;
  } else if (!startDetected) {
    if (toneMatch) {
      if (++startMatches >= 2) startDetected = true;
    } else startMatches = 0;
  }
  // Update classification before the start event so both confirmation windows count.
  char candidate = timingOK ? r.candidate : '?';
  if (candidate != '?' && candidate == previousCandidate) {
    if (consecutiveMatches < 2) ++consecutiveMatches;
  } else consecutiveMatches = candidate == '?' ? 0 : 1;
  previousCandidate = candidate;
  const char *soundClass = consecutiveMatches >= 2 ? (candidate == 'A' ? "A" : "B") : "UNKNOWN";
  const char *classReason = !timingOK ? "TIMING" :
      (candidate != '?' && consecutiveMatches < 2 ? "NEED_SECOND_WINDOW" : r.classReason);
  if (serviceStopCommand()) return;
  if (startDetected) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("Start:1 Motor:START_REQUEST");
    Serial.print(" ESC_us:"); Serial.print(START_ESC_PULSE_US);
    Serial.print(" Duration_ms:"); Serial.print(MOTOR_TEST_MS);
    Serial.print(" Class:"); Serial.print(soundClass);
    Serial.print(" Candidate:"); Serial.print(candidate);
    Serial.print(" ClassReason:"); Serial.print(classReason);
    Serial.print(" Mod_Hz:"); Serial.print(r.modulationHz, 0);
    Serial.print(" ModDepth:"); Serial.print(r.modulationDepth, 2);
    Serial.print(" Score_A:"); Serial.print(r.scoreA, 2);
    Serial.print(" Score_B:"); Serial.println(r.scoreB, 2);
    startMotor();
    if (!motorRunning) Serial.println("Motor:FAULT Disconnect_ESC_power:1");
    return; // Do not delay timeout service with further measurements.
  }
  // startDetected is decided and retained entirely on Arduino.
  // A reset only re-arms detection; it does not issue a start command.
  int digitalValue = digitalRead(7);
  digitalWrite(LED_BUILTIN, startDetected ? HIGH : LOW);

  if (millis()-lastRangeMs >= RANGE_INTERVAL_MS) {
    lastRangeMs = millis();
    distanceCm = readDistanceCm();
  }
  // Keep existing monitor fields and add the classifier diagnostics.
  Serial.print("Mean_V:"); Serial.print(r.mean*5.0f/4095, 3);
  Serial.print("\tPeakToPeak_V:"); Serial.print(r.peakToPeak*5.0f/4095, 3);
  Serial.print("\tDO:"); Serial.print(digitalValue);
  Serial.print("\tDistance_cm:"); Serial.print(distanceCm, 1);
  Serial.print("\tClass:"); Serial.print(soundClass);
  Serial.print("\tCandidate:"); Serial.print(candidate);
  Serial.print("\tMod_Hz:"); Serial.print(r.modulationHz, 0);
  Serial.print("\tTone:"); Serial.print(r.toneRatio, 2);
  Serial.print("\tPurity:"); Serial.print(r.purity, 2);
  Serial.print("\tModDepth:"); Serial.print(r.modulationDepth, 2);
  Serial.print("\tScore_A:"); Serial.print(r.scoreA, 2);
  Serial.print("\tScore_B:"); Serial.print(r.scoreB, 2);
  Serial.print("\tClassReason:"); Serial.print(classReason);
  Serial.print("\tRMS:"); Serial.print(r.rms, 1);
  Serial.print("\tTiming_OK:"); Serial.print(timingOK ? 1 : 0);
  Serial.print("\tClipped:"); Serial.print(r.clipped ? 1 : 0);
  Serial.print("\tSample_Hz:"); Serial.print(sampleHz, 0);
  Serial.print("\tMaxLate_us:"); Serial.print(maxLateUs);
  Serial.print("\tLateSamples:"); Serial.print(lateSamples);
  Serial.print("\tStart:"); Serial.print(startDetected ? 1 : 0);
  Serial.print("\tState:"); Serial.print(startDetected ? "STARTED" : !armed ? "WAIT_CLEAR" : startMatches ? "CONFIRMING" : "ARMED");
  Serial.print("\tReason:"); Serial.println(reason);
}
// One test per reset. Reset holds neutral and requires a fresh quiet-then-tone sequence.
// Sound processing and pulseIn block only while waiting at neutral. A stop command
// during waiting is serviced after the current audio block/processing (not instantly).
// Physical power disconnect remains the fallback if firmware or PWM output fails.
