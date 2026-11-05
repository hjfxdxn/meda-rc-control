#pragma once
#include <ArduinoBLE.h>
#include "remote_control.h"

// Custom GATT service. All command and response payloads fit the default 20-byte MTU.
BLEService controlService("a1500000-4d45-4441-8000-008200000001");
BLEStringCharacteristic commandChar("a1500001-4d45-4441-8000-008200000001",BLEWrite,20);
BLECharacteristic statusChar("a1500002-4d45-4441-8000-008200000001",BLERead|BLENotify,20,true);
BLEStringCharacteristic replyChar("a1500003-4d45-4441-8000-008200000001",BLERead|BLENotify,20);
bool bleReady=false, wasConnected=false, hardStop=false, remoteAuto=false;
uint8_t controlMode=2; // 0 idle, 1 manual, 2 automatic.
int manualThrottle=0,manualSteer=0,reverseUs=REVERSE_TEST_PULSE_US;
uint32_t outputInitializedMs=0,lastBleStatusMs=0,lastHeartbeatMs=0;
remote::Deadman manualLease;

bool actuatorConfigReady() {
  return config.neutralUs>=1000 && config.neutralUs<=2000 && config.forwardUs>=1000 && config.forwardUs<=2000 &&
    reverseUs>=1000 && reverseUs<=2000 && config.leftUs>=900 && config.rightUs<=2100 &&
    config.leftUs<config.centerUs && config.centerUs<config.rightUs &&
    (config.car==83 ? config.forwardUs>config.neutralUs && reverseUs<config.neutralUs :
      config.forwardUs<config.neutralUs && reverseUs>config.neutralUs);
}
void latchStop(race::Reason why) {
  hardStop=true;controlMode=0;manualThrottle=manualSteer=0;manualLease.clear();
  controller.stop(why);
}
void serviceRemoteDeadline() {
  if (controlMode==1 && manualLease.expired(millis(),700)) latchStop(race::Reason::UserStop);
  if (controlMode==2 && remoteAuto && uint32_t(millis()-lastHeartbeatMs)>=1500) latchStop(race::Reason::UserStop);
}
void initializeOutputsIfReady() {
  if (!ENABLE_MOTOR_OUTPUT || !actuatorConfigReady() || hardStop) return;
  if (!escReady && !steeringReady) {
    escReady=esc.begin(10000,uint32_t(config.neutralUs));
    steeringReady=steering.begin(20000,uint32_t(config.centerUs));
    outputInitializedMs=millis();lastEscUs=lastSteeringUs=-1;
    if (!escReady || !steeringReady) latchStop(race::Reason::OutputFault);
  }
}
bool changeParameter(const char* key,int v) {
  if (controlMode!=0 || hardStop) return false;
  // Actuator calibration cannot change while PWM is initialized. Reset first.
  if (!strcmp(key,"n") || !strcmp(key,"f") || !strcmp(key,"r") || !strcmp(key,"c") || !strcmp(key,"l") || !strcmp(key,"u")) {
    if (escReady || steeringReady || v<900 || v>2100) return false;
    if ((!strcmp(key,"n")||!strcmp(key,"f")||!strcmp(key,"r")) && (v<1000||v>2000)) return false;
    if (!strcmp(key,"n")) config.neutralUs=v;
    if (!strcmp(key,"f")) config.forwardUs=v;
    if (!strcmp(key,"r")) reverseUs=v;
    if (!strcmp(key,"c")) config.centerUs=v;
    if (!strcmp(key,"l")) config.leftUs=v;
    if (!strcmp(key,"u")) config.rightUs=v;
  } else if (!strcmp(key,"stop") && v>=2 && v<400) config.stopCm=float(v);
  else if (!strcmp(key,"side") && v>=2 && v<=400) config.sideTargetCm=float(v);
  else if (!strcmp(key,"gain") && v>=1 && v<=200) config.sideGainUsPerCm=v/10.0f;
  else if (!strcmp(key,"limit") && v>=1 && v<=400) config.maxCorrectionUs=v;
  else if (!strcmp(key,"time") && v>=1 && v<=300) config.maxRunMs=uint32_t(v)*1000;
  else if (!strcmp(key,"win") && v>=1 && v<=20) config.matchWindows=unsigned(v);
  else if (!strcmp(key,"sideR") && (v==0||v==1)) config.roles[0]=v?race::Role::Right:race::Role::Left;
  else return false;
  // Remote edits never assert course/hardware commissioning.
  controller=race::Controller(config);controller.stop(race::Reason::None);
  return true;
}
int parameterValue(const char* key) {
  if (!strcmp(key,"n"))return config.neutralUs;if (!strcmp(key,"f"))return config.forwardUs;
  if (!strcmp(key,"r"))return reverseUs;if (!strcmp(key,"c"))return config.centerUs;
  if (!strcmp(key,"l"))return config.leftUs;if (!strcmp(key,"u"))return config.rightUs;
  if (!strcmp(key,"stop"))return int(config.stopCm);if (!strcmp(key,"side"))return int(config.sideTargetCm);
  if (!strcmp(key,"gain"))return int(config.sideGainUsPerCm*10);if (!strcmp(key,"limit"))return config.maxCorrectionUs;
  if (!strcmp(key,"time"))return int(config.maxRunMs/1000);if (!strcmp(key,"win"))return int(config.matchWindows);
  if (!strcmp(key,"sideR"))return config.roles[0]==race::Role::Right?1:0;
  return -1;
}
void handleBleCommand(const char* text) {
  // Bound digit runs before sscanf so integer conversion cannot overflow.
  unsigned digits=0;
  if(strlen(text)>20) {replyChar.writeValue("ERR length");return;}
  for(const char* p=text;*p;++p) {
    digits=(*p>='0'&&*p<='9')?digits+1:0;
    if(digits>6) {replyChar.writeValue("ERR number");return;}
  }
  char key[9]={},extra=0;int a=0,b=0;
  const char* reply="ERR command";
  if (!strcmp(text,"STOP")) { latchStop(race::Reason::UserStop);applyOutputs();reply="OK STOP"; }
  else if (!strcmp(text,"H")) { lastHeartbeatMs=millis();reply="OK H"; }
  else if (!strncmp(text,"G,",2) && strlen(text+2)<sizeof(key)) {
    const int value=parameterValue(text+2);char response[20];
    snprintf(response,sizeof(response),"V,%s,%d",text+2,value);replyChar.writeValue(response);return;
  } else if (hardStop) reply="ERR reset required";
  else if (!strcmp(text,"IDLE")) {
    controlMode=0;manualThrottle=manualSteer=0;manualLease.clear();remoteAuto=false;
    controller.stop(race::Reason::None);applyOutputs();reply="OK IDLE";
  } else if (!strcmp(text,"MANUAL")) {
    if(controlMode!=0) reply="ERR select IDLE";
    else { initializeOutputsIfReady();
      if (!escReady || !steeringReady || hardStop) reply="ERR output disabled";
      else if(uint32_t(millis()-outputInitializedMs)<3000) reply="ERR settling";
      else {controlMode=1;manualThrottle=manualSteer=0;manualLease.command(millis());reply="OK MANUAL";}
    }
  } else if (!strcmp(text,"AUTO")) {
    if(controlMode!=0) reply="ERR select IDLE";
    else {initializeOutputsIfReady();
      if (!race::validConfig(config) || !escReady || !steeringReady || hardStop) reply="ERR configuration";
      else {controlMode=2;remoteAuto=true;lastHeartbeatMs=millis();controller=race::Controller(config);controller.begin(millis());sampled[0]=sampled[1]=false;reply="OK AUTO";}
    }
  } else if (sscanf(text,"M,%d,%d%c",&a,&b,&extra)==2 && a>=-100 && a<=100 && b>=-100 && b<=100) {
    if(controlMode!=1) reply="ERR not manual";
    else {manualThrottle=a;manualSteer=b;manualLease.command(millis());applyOutputs();reply="OK M";}
  } else if (sscanf(text,"P,%8[^,],%d%c",key,&a,&extra)==2) reply=changeParameter(key,a)?"OK P":"ERR parameter";
  applyOutputs();replyChar.writeValue(reply);
}
void startBle() {
  bleReady=BLE.begin();if(!bleReady)return;
  String address=BLE.address();address.replace(":","");
  String name=String("MEDA-82-")+address.substring(address.length()>4?address.length()-4:0);
  BLE.setLocalName(name.c_str());BLE.setAdvertisedService(controlService);
  controlService.addCharacteristic(commandChar);controlService.addCharacteristic(statusChar);controlService.addCharacteristic(replyChar);
  BLE.addService(controlService);replyChar.writeValue("READY");BLE.advertise();
}
void serviceBle() {
  if (!bleReady || pingPhase) return; // Do not block echo edge polling.
  BLE.poll();const bool connected=BLE.connected();
  if (wasConnected && !connected) {
    if(controlMode==1 || (controlMode==2 && remoteAuto)) latchStop(race::Reason::UserStop);
    applyOutputs();BLE.advertise();
  }
  wasConnected=connected;
  // Check deadlines before accepting a newly arrived late control packet.
  serviceRemoteDeadline();applyOutputs();
  if(connected && commandChar.written()) {
    const String value=commandChar.value();
    if(value.length()>0 && value.length()<=20 && strlen(value.c_str())==value.length()) handleBleCommand(value.c_str());
    else replyChar.writeValue("ERR length");
  }
  if(connected && uint32_t(millis()-lastBleStatusMs)>=250) {
    lastBleStatusMs=millis();uint8_t packet[20]={1,controlMode,uint8_t(controller.state),uint8_t(controller.reason)};
    packet[4]=(escReady&&steeringReady?1:0)|(hardStop?2:0)|(ENABLE_MOTOR_OUTPUT?4:0)|(config.confirmed?8:0);
    packet[5]=uint8_t(config.car);
    auto put=[&](int at,uint16_t v){packet[at]=uint8_t(v);packet[at+1]=uint8_t(v>>8);};
    put(6,ranges[1].valid?uint16_t(ranges[1].cm*10):65535);put(8,ranges[0].valid?uint16_t(ranges[0].cm*10):65535);
    put(10,lastEscUs>=0?uint16_t(lastEscUs):0);put(12,lastSteeringUs>=0?uint16_t(lastSteeringUs):0);
    put(14,uint16_t(latestRms*10));put(16,uint16_t(latestTone*1000));packet[18]=uint8_t(latestCandidate);
    packet[19]=config.roles[0]==race::Role::Right?1:0;statusChar.writeValue(packet,sizeof(packet));
  }
}
