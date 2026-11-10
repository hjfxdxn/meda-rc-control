#pragma once
#include "remote_control.h"
struct CommandReply {
 char text[24] = "READY";
 void writeValue(const char* value) {snprintf(text,sizeof(text),"%s",value);}
} replyChar;
bool hardStop=false,remoteAuto=false;
uint8_t controlMode=2;
// Car 82 development presets: only 1430 us is a reported starting threshold.
int speedPulses[3]={1200,1410,1380};
uint8_t selectedSpeed=0;
bool soundTest=false,soundTestArmed=false,soundTestDetected=false;
unsigned soundClear=0,soundMatches=0;
void serviceSoundTest(bool valid,bool match) {
 if(!soundTest||hardStop||soundTestDetected)return;
 if(!soundTestArmed){if(valid&&!match){if(++soundClear>=3)soundTestArmed=true;}else soundClear=0;}
 else {if(valid&&match){if(++soundMatches>=config.matchWindows)soundTestDetected=true;}else soundMatches=0;}
}
int manualThrottle=0,manualSteer=0,reverseUs=REVERSE_TEST_PULSE_US;
uint32_t outputInitializedMs=0,lastHeartbeatMs=0;
remote::Deadman manualLease;
bool actuatorConfigReady() {
  return config.neutralUs>=1000 && config.neutralUs<=2000 && config.forwardUs>=1000 && config.forwardUs<=2000 &&
    reverseUs>=1000 && reverseUs<=2000 && config.leftUs>=900 && config.rightUs<=2100 &&
    config.leftUs<config.centerUs && config.centerUs<config.rightUs &&
    (config.car==83 ? config.forwardUs>config.neutralUs && reverseUs<config.neutralUs :
      config.forwardUs<config.neutralUs && reverseUs>config.neutralUs);
}
void latchStop(race::Reason why) {
  soundTest=false;
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
  } else if ((!strcmp(key,"s0")||!strcmp(key,"s1")||!strcmp(key,"s2")) && v>=1000 && v<=2000) {
    if(config.car!=82||v>=config.neutralUs||v>1430)return false;
    const int i=key[1]-'0';speedPulses[i]=v;
    if(selectedSpeed==i)config.forwardUs=v;
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
  if(!strcmp(key,"s0")||!strcmp(key,"s1")||!strcmp(key,"s2"))return speedPulses[key[1]-'0'];
  if (!strcmp(key,"n"))return config.neutralUs;if (!strcmp(key,"f"))return config.forwardUs;
  if (!strcmp(key,"r"))return reverseUs;if (!strcmp(key,"c"))return config.centerUs;
  if (!strcmp(key,"l"))return config.leftUs;if (!strcmp(key,"u"))return config.rightUs;
  if (!strcmp(key,"stop"))return int(config.stopCm);if (!strcmp(key,"side"))return int(config.sideTargetCm);
  if (!strcmp(key,"gain"))return int(config.sideGainUsPerCm*10);if (!strcmp(key,"limit"))return config.maxCorrectionUs;
  if (!strcmp(key,"time"))return int(config.maxRunMs/1000);if (!strcmp(key,"win"))return int(config.matchWindows);
  if (!strcmp(key,"sideR"))return config.roles[0]==race::Role::Right?1:0;
  return -1;
}
void handleRemoteCommand(const char* text) {
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
    soundTest=false;
    controlMode=0;manualThrottle=manualSteer=0;manualLease.clear();remoteAuto=false;
    controller.stop(race::Reason::None);applyOutputs();reply="OK IDLE";
  } else if (!strcmp(text,"SOUND")) {
    if(controlMode!=0)reply="ERR select IDLE";
    else {soundTest=true;soundTestArmed=soundTestDetected=false;soundClear=soundMatches=0;sampled[0]=sampled[1]=false;reply="OK SOUND TEST";}
  } else if(sscanf(text,"SPEED,%d%c",&a,&extra)==1 && a>=0 && a<=2) {
    if(controlMode!=0)reply="ERR select IDLE";
    else if(config.car!=82||speedPulses[a]>=config.neutralUs)reply="ERR configuration";
    else {selectedSpeed=uint8_t(a);config.forwardUs=speedPulses[a];controller=race::Controller(config);controller.stop(race::Reason::None);reply="OK SPEED";}
  } else if (!strcmp(text,"MANUAL")) {
    if(controlMode!=0) reply="ERR select IDLE";
    else { initializeOutputsIfReady();
      if (!escReady || !steeringReady || hardStop) reply="ERR output disabled";
      else if(uint32_t(millis()-outputInitializedMs)<3000) reply="ERR settling";
      else {soundTest=false;controlMode=1;manualThrottle=manualSteer=0;manualLease.command(millis());reply="OK MANUAL";}
    }
  } else if (!strcmp(text,"AUTO")) {
    if(controlMode!=0) reply="ERR select IDLE";
    else {initializeOutputsIfReady();
      if (!race::validConfig(config) || !escReady || !steeringReady || hardStop) reply="ERR configuration";
      else {soundTest=false;controlMode=2;remoteAuto=false;lastHeartbeatMs=millis();controller=race::Controller(config);controller.begin(millis());sampled[0]=sampled[1]=false;reply="OK AUTO";}
    }
  } else if (sscanf(text,"M,%d,%d%c",&a,&b,&extra)==2 && a>=-100 && a<=100 && b>=-100 && b<=100) {
    if(controlMode!=1) reply="ERR not manual";
    else {manualThrottle=a;manualSteer=b;manualLease.command(millis());applyOutputs();reply="OK M";}
  } else if (sscanf(text,"P,%8[^,],%d%c",key,&a,&extra)==2) reply=changeParameter(key,a)?"OK P":"ERR parameter";
  applyOutputs();replyChar.writeValue(reply);
}
