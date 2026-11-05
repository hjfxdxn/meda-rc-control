#pragma once
#include <stddef.h>
#include <stdint.h>
struct IPAddress {IPAddress(int,int,int,int){}};
constexpr int WL_NO_MODULE=0,WL_AP_LISTENING=1;
class WiFiClient {public: operator bool()const{return false;}void stop(){}int available(){return 0;}int read(){return -1;}size_t write(const uint8_t*,size_t n){return n;}};
class WiFiServer {public:explicit WiFiServer(int){}void begin(){}WiFiClient available(){return WiFiClient();}};
class FakeWiFi {public:int scanNetworks(){return 0;}int status(){return WL_AP_LISTENING;}void macAddress(uint8_t* p){for(int i=0;i<6;i++)p[i]=0;}void config(IPAddress){}int beginAP(const char*,const char*){return WL_AP_LISTENING;}};
static FakeWiFi WiFi;
