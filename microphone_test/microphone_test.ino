// UNO R4 WiFi: autonomous common A/B start-tone detection + HC-SR04 monitor.
// Microphone: AO=A0, DO=D7, 5V-compatible VCC=5V, GND=GND.
// HC-SR04 default GPIO mode: Trig=D9, Echo=D11, VCC=5V, GND=GND.
// Classification uses AO waveform; DO and its threshold do not decide A/B.
#include "ra0020_classifier.h"

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
// START is latched until reset. This sketch never commands the ESC.
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
}

void loop() {
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
  // Integration point: startDetected is decided and retained entirely on Arduino.
  // A reset only re-arms detection; it does not issue a start command.
  char candidate = timingOK ? r.candidate : '?';
  if (candidate != '?' && candidate == previousCandidate) {
    if (consecutiveMatches < 2) ++consecutiveMatches;
  } else consecutiveMatches = candidate == '?' ? 0 : 1;
  previousCandidate = candidate;
  const char *soundClass = consecutiveMatches >= 2 ? (candidate == 'A' ? "A" : "B") : "Unknown";
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
// Bench test only: no servo/ESC control. Sampling blocks for 256 ms; processing,
// Serial output and ultrasound create gaps between blocks. Not continuous audio.
// START and LED remain active until reset; Class A/B is optional diagnostic output.
