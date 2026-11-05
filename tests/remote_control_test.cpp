#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../race_150m/race_control.h"
struct TestSerial{template<class T>void print(T){}template<class T>void println(T){}} Serial;
void delay(unsigned long){}
uint32_t clockMs=0;uint32_t millis(){return clockMs;}
constexpr int REVERSE_TEST_PULSE_US=0;
constexpr bool ENABLE_MOTOR_OUTPUT=true; // Test fixture only.
race::Config config;
race::Controller controller(config);
struct Pwm {bool begin(uint32_t,uint32_t){return true;}} esc,steering;
bool escReady=false,steeringReady=false,sampled[2]={};
int lastEscUs=-1,lastSteeringUs=-1;uint8_t pingPhase=0;
race::Range ranges[2];float latestRms=0,latestTone=0;char latestCandidate='?';
void applyOutputs(){}
#include "../race_150m/command_control.h"
#include "../race_150m/wifi_control.h"
void resetFixture(){config=race::Config{};config.car=82;config.neutralUs=1530;config.forwardUs=1400;reverseUs=1650;config.centerUs=1550;config.leftUs=1150;config.rightUs=1950;hardStop=false;controlMode=0;escReady=steeringReady=false;manualLease.clear();remoteAuto=false;clockMs=0;controller=race::Controller(config);}
void expect(const char* value){assert(!strcmp(replyChar.text,value));}
int main(){
 assert(remote::mapPercent(100,1530,1650,1400)==1400);
 assert(remote::mapPercent(-100,1530,1650,1400)==1650);
 assert(remote::mapPercent(-150,1700,1150,1950)==1150);
 assert(remote::mapPercent(150,1700,1150,1950)==1950);
 resetFixture();handleRemoteCommand("M,100,0");expect("ERR not manual");
 handleRemoteCommand("MANUAL");expect("ERR settling");clockMs=3000;
 handleRemoteCommand("MANUAL");expect("OK MANUAL");assert(controlMode==1);
 handleRemoteCommand("M,25,-50");expect("OK M");assert(manualThrottle==25&&manualSteer==-50);
 handleRemoteCommand("M,101,0");expect("ERR command");assert(manualThrottle==25);
 handleRemoteCommand("P,n,1500");expect("ERR parameter");assert(config.neutralUs==1530);
 clockMs=3700;serviceRemoteDeadline();assert(hardStop&&controlMode==0);
 handleRemoteCommand("MANUAL");expect("ERR reset required");handleRemoteCommand("AUTO");expect("ERR reset required");
 resetFixture();handleRemoteCommand("P,n,1500");expect("OK P");assert(config.neutralUs==1500);
 handleRemoteCommand("P,n,999999999999");expect("ERR number");assert(config.neutralUs==1500);
 handleRemoteCommand("P,sideR,1");expect("OK P");assert(config.roles[0]==race::Role::Right);
 handleRemoteCommand("G,n");expect("V,n,1500");
 handleRemoteCommand("AUTO");expect("ERR configuration");assert(controlMode==0);
 resetFixture();handleRemoteCommand("STOP");expect("OK STOP");assert(hardStop);
 handleRemoteCommand("IDLE");expect("ERR reset required");
 resetFixture();config.confirmed=true;config.roles[0]=race::Role::Left;config.roles[1]=race::Role::Front;
 config.matchWindows=2;config.stopCm=80;config.sideTargetCm=60;config.sideGainUsPerCm=2;config.maxCorrectionUs=50;config.maxRunMs=5000;
 handleRemoteCommand("AUTO");expect("OK AUTO");assert(controlMode==2&&!remoteAuto);
 handleRemoteCommand("M,100,0");expect("ERR not manual");
 clockMs=1500;serviceRemoteDeadline();assert(!hardStop);

 resetFixture();manualLease.command(0xFFFFFF00u);controlMode=1;clockMs=0xFFFFFF00u+700;serviceRemoteDeadline();assert(hardStop);
 resetFixture();ownerToken=0;strcpy(request,"POST /claim HTTP/1.1");handleHttp();assert(ownerToken!=0);
 const uint32_t token=ownerToken;
 handleHttp();assert(strstr(responseText,"busy"));
 snprintf(request,sizeof(request),"POST /cmd?token=%lu&seq=1&c=P,side,75 HTTP/1.1",(unsigned long)token);handleHttp();assert(config.sideTargetCm==75);
 snprintf(request,sizeof(request),"POST /cmd?token=%lu&seq=1&c=P,side,80 HTTP/1.1",(unsigned long)token);handleHttp();assert(config.sideTargetCm==75);
 snprintf(request,sizeof(request),"POST /cmd?token=%lu&seq=2&c=STOP HTTP/1.1",(unsigned long)(token+1));handleHttp();assert(!hardStop);
 snprintf(request,sizeof(request),"POST /cmd?token=%lu&seq=2&c=STOP HTTP/1.1",(unsigned long)token);handleHttp();assert(hardStop);
 resetFixture();ownerToken=token;ownerSeen=0;controlMode=1;wifiReady=true;clockMs=3001;serviceWifi();assert(hardStop&&ownerToken==0);
 resetFixture();uint8_t mac[6]={0xD8,0x85,0xAC,0xA8,0xE9,0x6C};assert(identity::originalCar(mac)==82);memset(mac,0,6);assert(!identity::validMac(mac));memset(mac,255,6);assert(!identity::validMac(mac));
 handleRemoteCommand("SPEED,0");expect("OK SPEED");assert(config.forwardUs==1430);
 handleRemoteCommand("SPEED,1");expect("OK SPEED");assert(config.forwardUs==1410);
 controlMode=1;handleRemoteCommand("SPEED,2");expect("ERR select IDLE");assert(config.forwardUs==1410);controlMode=0;
 config.matchWindows=2;handleRemoteCommand("SOUND");expect("OK SOUND TEST");
 serviceSoundTest(true,true);assert(!soundTestArmed);for(int i=0;i<3;i++)serviceSoundTest(true,false);assert(soundTestArmed);
 serviceSoundTest(true,true);assert(!soundTestDetected);serviceSoundTest(true,true);assert(soundTestDetected&&controlMode==0);
 handleRemoteCommand("STOP");handleRemoteCommand("SOUND");expect("ERR reset required");
 resetFixture();ownerToken=0;controlMode=2;controller.state=race::State::Running;
 strcpy(request,"POST /claim HTTP/1.1");handleHttp();assert(ownerToken&&controlMode==2&&controller.state==race::State::Running);
 ownerSeen=0;clockMs=4000;serviceWifi();assert(!hardStop&&ownerToken==0);
 strcpy(request,"POST /stop HTTP/1.1");handleHttp();assert(hardStop&&controller.state==race::State::Stopped);
 puts("PASS: Remote command gates, calibration lock, bounds, manual scaling, stop latch, deadman and heartbeat.");
}
