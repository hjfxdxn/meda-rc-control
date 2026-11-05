#pragma once
#include <WiFiS3.h>
#include "wifi_panel_page.h"
#include "board_identity.h"
WiFiServer raceServer(80);
WiFiClient httpClient;
bool wifiReady=false;
char apName[32]="MEDA-Race82";
constexpr char AP_PASSWORD[]="MedaRace2026";
uint32_t ownerToken=0,tokenCounter=0,ownerSequence=0,ownerSeen=0,requestAt=0;
char request[192];size_t requestLength=0,headerBytes=0;
bool firstLine=true;
char previousHeader=0;
uint8_t headerEnd=0;
const uint8_t* responseBody=nullptr;size_t responseLength=0,responseSent=0;
char responseHeader[200];size_t headerLength=0,headerSent=0;
char responseText[640];uint8_t statusPacket[20];
bool httpBusy(){return bool(httpClient);}
void closeHttp(){httpClient.stop();responseBody=nullptr;responseLength=responseSent=0;headerLength=headerSent=0;}
void respond(const uint8_t* body,size_t length,const char* type,int status=200){
 responseBody=body;responseLength=length;responseSent=0;
 headerLength=size_t(snprintf(responseHeader,sizeof(responseHeader),"HTTP/1.1 %d %s\r\nConnection: close\r\nCache-Control: no-store\r\nContent-Type: %s\r\nContent-Length: %u\r\n\r\n",status,status==200?"OK":"Conflict",type,unsigned(length)));headerSent=0;
}
void respondText(const char* text,int status=200){snprintf(responseText,sizeof(responseText),"%s",text);respond((const uint8_t*)responseText,strlen(responseText),"text/plain",status);}
void revokeOwner(){ownerToken=0;ownerSequence=0;}
char stationMacText[18]="UNAVAILABLE",apMacText[18]="UNAVAILABLE";
int boardOriginalCar=0;
void formatMac(const uint8_t* mac,char* out){snprintf(out,18,"%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);}
bool readValidMac(uint8_t* mac){
 for(int attempt=0;attempt<3;++attempt){memset(mac,0,6);WiFi.macAddress(mac);if(identity::validMac(mac))return true;delay(20);}
 return false;
}
void startWifi(){
 if(WiFi.status()==WL_NO_MODULE)return;
 // WiFiS3 macAddress is meaningful only after scan/begin/beginAP sets a mode.
 Serial.println("WIFI_STATION_ID_BEGIN");
 WiFi.scanNetworks();
 uint8_t sta[6]={},ap[6]={};
 const bool stationValid=readValidMac(sta);
 if(stationValid){formatMac(sta,stationMacText);boardOriginalCar=identity::originalCar(sta);}
 if(stationValid)snprintf(apName,sizeof(apName),"MEDA-Race82-%02X%02X%02X",sta[3],sta[4],sta[5]);
 else snprintf(apName,sizeof(apName),"MEDA-Race82-MACUNKNOWN");
 WiFi.config(IPAddress(192,168,4,1));
 wifiReady=WiFi.beginAP(apName,AP_PASSWORD)==WL_AP_LISTENING;
 if(wifiReady){if(readValidMac(ap))formatMac(ap,apMacText);raceServer.begin();}
 Serial.print("Station_MAC:");Serial.println(stationMacText);
 Serial.print("AP_MAC:");Serial.println(apMacText);
 Serial.print("Original_board_car:");Serial.println(boardOriginalCar);
 Serial.println("Active_vehicle:82 (explicit configuration, independent of board MAC)");
}
void makeStatus(){
 memset(statusPacket,0,20);statusPacket[0]=1;statusPacket[1]=controlMode;statusPacket[2]=uint8_t(controller.state);statusPacket[3]=uint8_t(controller.reason);
 statusPacket[4]=(escReady&&steeringReady?1:0)|(hardStop?2:0)|(ENABLE_MOTOR_OUTPUT?4:0)|(config.confirmed?8:0);statusPacket[5]=82;
 auto put=[&](int i,uint16_t v){statusPacket[i]=uint8_t(v);statusPacket[i+1]=uint8_t(v>>8);};
 put(6,ranges[1].valid?uint16_t(ranges[1].cm*10):65535);put(8,ranges[0].valid?uint16_t(ranges[0].cm*10):65535);
 put(10,lastEscUs>=0?uint16_t(lastEscUs):0);put(12,lastSteeringUs>=0?uint16_t(lastSteeringUs):0);
 put(14,uint16_t(latestRms*10));put(16,uint16_t(latestTone*1000));statusPacket[18]=uint8_t(latestCandidate);statusPacket[19]=config.roles[0]==race::Role::Right?1:0;
}
void handleHttp(){
 serviceRemoteDeadline();applyOutputs();
 if(!strcmp(request,"GET / HTTP/1.1")||!strcmp(request,"GET /wifi_panel.html HTTP/1.1")){
  if(controlMode==1){respondText("Stop manual driving before loading the page.",409);return;}
  respond((const uint8_t*)WIFI_PANEL_HTML,strlen(WIFI_PANEL_HTML),"text/html; charset=utf-8");return;
 }
 if(!strcmp(request,"GET /info HTTP/1.1")){
  snprintf(responseText,sizeof(responseText),"{\"stationMac\":\"%s\",\"apMac\":\"%s\",\"originalCar\":%d,\"activeCar\":82,\"speed\":%u,\"pulses\":[%d,%d,%d],\"forwardUs\":%d,\"soundTest\":%u,\"soundArmed\":%u,\"soundDetected\":%u}",stationMacText,apMacText,boardOriginalCar,unsigned(selectedSpeed),speedPulses[0],speedPulses[1],speedPulses[2],config.forwardUs,unsigned(soundTest),unsigned(soundTestArmed),unsigned(soundTestDetected));
  respond((uint8_t*)responseText,strlen(responseText),"application/json");return;
 }
 if(!strcmp(request,"GET /status HTTP/1.1")){makeStatus();respond(statusPacket,20,"application/octet-stream");return;}
 // Any client already on the password-protected AP may request STOP.
 if(!strcmp(request,"POST /stop HTTP/1.1")){
  latchStop(race::Reason::UserStop);applyOutputs();respondText("OK STOP");return;
 }
 if(!strcmp(request,"POST /claim HTTP/1.1")){
  if(ownerToken||controlMode==1){respondText("ERR control busy",409);return;}
  ownerToken=++tokenCounter;if(!ownerToken)ownerToken=++tokenCounter;ownerSequence=0;ownerSeen=millis();
  // Connecting observes the existing autonomous state; it does not disarm it.
  snprintf(responseText,sizeof(responseText),"TOKEN:%lu",(unsigned long)ownerToken);respond((uint8_t*)responseText,strlen(responseText),"text/plain");return;
 }
 unsigned long token=0,seq=0;char cmd[21]={};int consumed=-1;
 if(sscanf(request,"POST /cmd?token=%lu&seq=%lu&c=%20[A-Za-z0-9,-] HTTP/1.1%n",&token,&seq,cmd,&consumed)!=3||consumed!=int(strlen(request))||!ownerToken||token!=ownerToken||seq<=ownerSequence){respondText("ERR session",409);return;}
 ownerSequence=seq;ownerSeen=millis();
 if(!strcmp(cmd,"RELEASE")){
  if(controlMode==1||(controlMode==2&&remoteAuto))latchStop(race::Reason::UserStop);
  applyOutputs();revokeOwner();respondText("OK RELEASE");return;
 }
 handleRemoteCommand(cmd);respondText(replyChar.text);
}
void serviceWifi(){
 if(!wifiReady||pingPhase)return;
 serviceRemoteDeadline();applyOutputs();
 if(ownerToken&&uint32_t(millis()-ownerSeen)>=3000){if(controlMode==1||(controlMode==2&&remoteAuto))latchStop(race::Reason::UserStop);applyOutputs();revokeOwner();}
 if(!httpClient){httpClient=raceServer.available();if(!httpClient)return;requestAt=millis();requestLength=headerBytes=0;firstLine=true;headerEnd=0;responseBody=nullptr;request[0]=0;}
 if(uint32_t(millis()-requestAt)>5000){closeHttp();return;}
 if(responseBody){
  const uint8_t* data;size_t left;
  if(headerSent<headerLength){data=(const uint8_t*)responseHeader+headerSent;left=headerLength-headerSent;}else{data=responseBody+responseSent;left=responseLength-responseSent;}
  if(left>256)left=256;
  const size_t sent=httpClient.write(data,left);
  if(headerSent<headerLength)headerSent+=sent;else responseSent+=sent;
  if(headerSent>=headerLength&&responseSent>=responseLength)closeHttp();
  return;
 }
 if(uint32_t(millis()-requestAt)>1500){closeHttp();return;}
 for(int budget=0;budget<256&&httpClient.available();++budget){
  char ch=char(httpClient.read());if(++headerBytes>4096||ch==0){closeHttp();return;}
  if(firstLine){if(ch=='\n'){firstLine=false;request[requestLength]=0;}else if(ch!='\r'){if(requestLength>=sizeof(request)-1){closeHttp();return;}request[requestLength++]=ch;}}
  const char ending[]="\r\n\r\n";headerEnd=ch==ending[headerEnd]?uint8_t(headerEnd+1):(ch=='\r'?1:0);
  if(headerEnd==4){handleHttp();return;}
 }
}
