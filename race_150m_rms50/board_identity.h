#pragma once
#include <stdint.h>
#include <string.h>
namespace identity {
inline bool validMac(const uint8_t* mac) {
 bool zero=true,ones=true;for(int i=0;i<6;++i){zero=zero&&mac[i]==0;ones=ones&&mac[i]==255;}
 return !zero&&!ones&&!(mac[0]&1);
}
inline int originalCar(const uint8_t* mac) {
 static const uint8_t known[3][6]={{0xF4,0x12,0xFA,0x70,0x5D,0x80},{0xD8,0x85,0xAC,0xA8,0xE9,0x6C},{0x34,0xB7,0xDA,0x63,0x8C,0x7C}};
 if(!validMac(mac))return 0;
 for(int i=0;i<3;++i)if(!memcmp(mac,known[i],6))return 81+i;
 return 0;
}
}
