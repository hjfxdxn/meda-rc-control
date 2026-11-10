// Autonomous 150 m development sketch. Standalone sound-start trial; parameters require track validation.
// English documentation: README.md. This is independent of start_test.
#include <Arduino.h>
#include <pwm.h>
#include "race_config.h"
#include <math.h>
#include <Arduino_LED_Matrix.h>
#include "sound_rms.h"

race::Config config = makeRaceConfig();
race::Controller controller(config);
PwmOut esc(D6), steering(D5);
bool escReady = false, steeringReady = false;
int lastEscUs = -1, lastSteeringUs = -1;
race::Range ranges[2];
constexpr uint8_t TRIG[2] = {9,8}, ECHO[2] = {11,12};
constexpr uint32_t PING_GAP_US = 60000, ECHO_LIMIT_US = 25000;
uint8_t activeSensor = 0, nextSensor = 0;
uint8_t pingPhase = 0; // 0 idle, 1 waiting for rise, 2 waiting for fall.
uint32_t pingAtUs = 0, riseAtUs = 0, lastPingUs = 0;
bool sampled[2] = {false,false};

uint32_t lastLogMs = 0;
uint32_t bootMs = 0;
bool wifiAttempted = false;

float latestRms = 0, latestTone = 0;
char latestCandidate = '?';
void applyOutputs();
#include "command_control.h"
#include "wifi_control.h"
#include "matrix_status.h"

void applyOutputs() {
  if (!escReady || !steeringReady) return;
  const bool running = !hardStop && controlMode==2 && controller.state == race::State::Running;
  const bool manual = !hardStop && controlMode==1;
  const int throttle = manual ? remote::mapPercent(manualThrottle,config.neutralUs,reverseUs,config.forwardUs) : running ? config.forwardUs : config.neutralUs;
  const int steer = manual ? remote::mapPercent(manualSteer,config.centerUs,config.leftUs,config.rightUs) : running ? controller.steeringUs : config.centerUs;
  bool a = true, b = true;
  if (throttle != lastEscUs) a = esc.pulseWidth_us(throttle);
  if (steer != lastSteeringUs) b = steering.pulseWidth_us(steer);
  if (!a || !b) {
    latchStop(race::Reason::OutputFault);
    // Attempt neutral on both channels independently. Failure needs power removal.
    esc.pulseWidth_us(config.neutralUs);
    steering.pulseWidth_us(config.centerUs);
    lastEscUs = lastSteeringUs = -1;
    return;
  }
  lastEscUs = throttle; lastSteeringUs = steer;
}
void serviceStop() {
  for (int i=0;i<8 && Serial.available();++i) {
    char c = char(Serial.read());
    if (c=='x' || c=='n' || c=='X' || c=='N') {
      latchStop(race::Reason::UserStop); applyOutputs();
      Serial.println("STOP_REQUESTED Reset_required:1");
    }
  }
}
void finishPing(bool valid, uint32_t pulseUs) {
  const float cm = pulseUs * 0.01715f;
  ranges[activeSensor].valid = valid && cm >= 2 && cm <= 400;
  ranges[activeSensor].cm = cm;
  ranges[activeSensor].atMs = millis();
  sampled[activeSensor] = true;
  pingPhase = 0;
}
void serviceSonar(bool permitNewPing) {
  const uint32_t now = micros();
  if (pingPhase) {
    // Timeout takes priority. A disconnected or stuck echo is never 'clear'.
    if (uint32_t(now-pingAtUs) >= ECHO_LIMIT_US) { finishPing(false,0); return; }
    const bool high = digitalRead(ECHO[activeSensor]) == HIGH;
    if (pingPhase==1 && high) { riseAtUs=now; pingPhase=2; }
    else if (pingPhase==2 && !high) finishPing(true,uint32_t(now-riseAtUs));
    return;
  }
  if (!permitNewPing || uint32_t(now-lastPingUs)<PING_GAP_US) return;
  activeSensor=nextSensor; nextSensor=uint8_t(1-nextSensor); lastPingUs=now;
  if (digitalRead(ECHO[activeSensor])==HIGH) { finishPing(false,0); return; }
  digitalWrite(TRIG[activeSensor],HIGH); delayMicroseconds(12);
  digitalWrite(TRIG[activeSensor],LOW);
  pingAtUs=micros(); pingPhase=1;
}
void sampleAudio() {
  // Audio only runs while stationary. No sonar transaction overlaps this block.
  SimpleRms accumulator;
  uint32_t due = micros();
  for (unsigned i = 0; i < 500; ++i) {
    while (int32_t(micros() - due) < 0) {}
    accumulator.add(analogRead(A0));
    due += 100;
    if ((i & 31) == 31) { serviceStop(); if (hardStop) return; }
  }
  const bool valid = true; // RMS only: no tone, clipping or sample-jitter gate.
  latestRms = accumulator.value();
  const bool match = latestRms >= SOUND_RMS_THRESHOLD;
  latestTone = 0; latestCandidate = '?'; // Compatibility fields; not analyzed.
  serviceStop();
  serviceSoundTest(valid,match);
  if(controlMode==2 && !hardStop && (!ENABLE_WIFI || wifiAttempted)) controller.audio(millis(),valid,match,ranges);
  applyOutputs();
  sampled[0]=sampled[1]=false;
}
void logStatus() {
  // Match the working start_test: ordinary Print calls, no connection/space gate.
  // Log only at neutral. Direct USB writes must not delay motion deadlines.
  if (pingPhase || controlMode==1 || controller.state==race::State::Running) return;
  if (uint32_t(millis()-lastLogMs)<500) return;
  lastLogMs=millis();
  Serial.print("MEDA150 RMS50 LED Uptime_ms:"); Serial.print(millis());
  Serial.print(" WiFi:");
  Serial.print(!ENABLE_WIFI?"DISABLED":!wifiAttempted?"PENDING":wifiReady?"READY":"FAILED");
  Serial.print(" State:");Serial.print(unsigned(controller.state));
  Serial.print(" Reason:");Serial.print(unsigned(controller.reason));
  Serial.print(" Front_cm:");Serial.print(ranges[1].valid?ranges[1].cm:-1.0f,1);
  Serial.print(" Side_cm:");Serial.print(ranges[0].valid?ranges[0].cm:-1.0f,1);
  Serial.print(" RMS:");Serial.print(latestRms,1);
  Serial.print(" Tone:");Serial.print(latestTone,2);
  Serial.print(" Class:");Serial.print(latestCandidate);
  Serial.print(" Speed:");Serial.print(unsigned(selectedSpeed));
  Serial.print(" Forward_us:");Serial.print(config.forwardUs);
  Serial.print(" SoundTest:");Serial.print(soundTestDetected?"DETECTED":soundTestArmed?"ARMED":soundTest?"WAIT_CLEAR":"OFF");
  Serial.print(" Output:");Serial.println(escReady&&steeringReady?1:0);
}
void serviceWifiStartup() {
  // Run sensors first. Keep radio initialization away from armed/running PWM.
  if (!ENABLE_WIFI || wifiAttempted || pingPhase || uint32_t(millis()-bootMs)<10000) return;
  Serial.println("WIFI_INIT_BEGIN");
  wifiAttempted=true;
  startWifi();
  Serial.println(wifiReady?"WIFI_INIT_OK":"WIFI_INIT_FAILED - sensor testing continues");
  if(wifiReady) {
    Serial.print("SSID:");Serial.println(apName);
    Serial.println("URL:http://192.168.4.1");
  }
  initializeOutputsIfReady();
  controller=race::Controller(config);controller.begin(millis());
  updateMatrixStatus();
  if(hardStop) controller.stop(race::Reason::UserStop);
}
void setup() {
  Serial.begin(115200); // Same USB serial initialization as the working start_test.
  Serial.println("MEDA150 RMS50 LED BOOT - standalone sound start enabled for car 82");
  pinMode(LED_BUILTIN,OUTPUT); pinMode(7,INPUT);
  for(int i=0;i<2;++i) { pinMode(TRIG[i],OUTPUT);digitalWrite(TRIG[i],LOW);pinMode(ECHO[i],INPUT); }
  analogReadResolution(12);
  Serial.println("AUDIO_INIT_BEGIN");
  // RMS-only detection needs no classifier initialization.
  Serial.println("AUDIO_INIT_OK");
  controller.begin(millis());
  bootMs=millis();
  // With Wi-Fi enabled, initialize actuator PWM only after the deferred radio attempt.
  if (!ENABLE_WIFI) initializeOutputsIfReady();
  controller.begin(millis());
  if(hardStop) controller.stop(race::Reason::OutputFault);
  lastPingUs=micros()-PING_GAP_US;
  Serial.println("SETUP_DONE - sensors first, Wi-Fi attempt after 10 seconds");
}
void loop() {
  serviceStop();serviceRemoteDeadline();
  logStatus();
  // Brief diagnostic window without sensor or network work after every reset.
  if(uint32_t(millis()-bootMs)<2000){digitalWrite(LED_BUILTIN,millis()%1000<100);return;}
  if(controlMode==2 && !hardStop) controller.tick(millis(),ranges);
  if(controller.state==race::State::Stopped && controller.reason!=race::Reason::None) latchStop(controller.reason);
  applyOutputs();
  const bool waiting = !hardStop && controlMode!=1 &&
    (controlMode==0 || controller.state==race::State::WaitingConfiguration ||
     controller.state==race::State::WaitingClear || controller.state==race::State::Armed);
  serviceSonar(!(waiting && sampled[0] && sampled[1]));
  if(controlMode==2 && !hardStop) controller.tick(millis(),ranges);
  if(controller.state==race::State::Stopped && controller.reason!=race::Reason::None) latchStop(controller.reason);
  applyOutputs();
  if(waiting && !httpBusy() && !pingPhase && sampled[0] && sampled[1]) sampleAudio();
  serviceWifi();
  serviceRemoteDeadline();applyOutputs();
  digitalWrite(LED_BUILTIN,controller.state==race::State::Running || controlMode==1 || millis()%1000<100 ? HIGH:LOW);
  logStatus();
  serviceWifiStartup();
  updateMatrixStatus();
}
