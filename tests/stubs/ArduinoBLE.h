#pragma once
#include <string>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
class String {
 std::string s;
 public:
 String(const char* p=""):s(p){} String(std::string p):s(p){}
 const char* c_str()const{return s.c_str();} size_t length()const{return s.size();}
 void replace(const char* a,const char* b){size_t p;while((p=s.find(a))!=std::string::npos)s.replace(p,strlen(a),b);}
 String substring(size_t p)const{return String(s.substr(p));}
 String operator+(const String& x)const{return String(s+x.s);}
};
constexpr int BLEWrite=1,BLERead=2,BLENotify=4;
class BLEStringCharacteristic {
 public:
 String stored;bool changed=false;
 BLEStringCharacteristic(const char*,int,int){}
 void writeValue(const char* text){stored=String(text);}
 String value()const{return stored;}
 bool written(){bool was=changed;changed=false;return was;}
};
class BLECharacteristic {
 public:
 uint8_t packet[20]={};
 BLECharacteristic(const char*,int,int,bool){}
 void writeValue(const uint8_t* p,size_t n){memcpy(packet,p,n);}
};
class BLEService {
 public: explicit BLEService(const char*){}
 template<class T>void addCharacteristic(T&){}
};
class FakeBLE {
 public:
 bool link=false;
 bool begin(){return true;}String address(){return String("AA:BB:CC:DD:EE:FF");}
 void setLocalName(const char*){} void setAdvertisedService(BLEService&){}void addService(BLEService&){}
 void advertise(){}void poll(){}bool connected(){return link;}
};
static FakeBLE BLE;
