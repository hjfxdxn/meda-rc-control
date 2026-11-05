#include "../race_150m/race_control.h"
#include <assert.h>
#include <stdio.h>
#include <limits>
using namespace race;
Config testConfig() {
 Config c; c.confirmed=true; c.car=82; c.neutralUs=1530;c.forwardUs=1420;
 c.centerUs=1550;c.leftUs=1150;c.rightUs=1950;
 c.roles[0]=Role::Front;c.roles[1]=Role::Left;c.matchWindows=2;
 c.stopCm=80;c.sideTargetCm=60;c.sideGainUsPerCm=2;c.maxCorrectionUs=50;c.maxRunMs=5000;
 return c;
}
void arm(Controller& c,Range (&r)[2],uint32_t base=0) {
 c.begin(base);c.tick(base+3000,r);
 c.audio(base+3000,true,false,r);c.audio(base+3000,true,false,r);c.audio(base+3000,true,false,r);
 assert(c.state==State::Armed);
}
void run(Controller& c,Range (&r)[2],uint32_t now=3000) {
 c.audio(now,true,true,r);assert(c.state==State::Armed);
 c.audio(now,true,true,r);assert(c.state==State::Running);
}
int main() {
 Config cfg=testConfig(); assert(validConfig(cfg)); assert(!validConfig(Config{}));
 Range r[2]={{true,200,3000},{true,60,3000}};
 Controller c(cfg);arm(c,r);run(c,r);
 r[1].cm=30;c.tick(3001,r);assert(c.steeringUs==1600);
 r[1].cm=100;c.tick(3002,r);assert(c.steeringUs==1500);
 r[0].cm=79;c.tick(3003,r);assert(c.reason==Reason::Obstacle);
 c.audio(3003,true,true,r);assert(c.state==State::Stopped);
 r[0].cm=200;
 Controller invalid(cfg);arm(invalid,r);r[0].valid=false;invalid.audio(3000,true,true,r);invalid.audio(3000,true,true,r);
 assert(invalid.reason==Reason::FrontRange);r[0].valid=true;
 Controller side(cfg);arm(side,r);run(side,r);r[1].valid=false;side.tick(3001,r);assert(side.reason==Reason::SideRange);r[1].valid=true;
 Controller stale(cfg);arm(stale,r);run(stale,r);stale.tick(3401,r);assert(stale.reason==Reason::FrontRange);
 Controller timeout(cfg);arm(timeout,r);run(timeout,r);timeout.tick(8000,r);assert(timeout.reason==Reason::TimeLimit);
 Controller cancel(cfg);arm(cancel,r);cancel.stop(Reason::UserStop);cancel.audio(3000,true,true,r);assert(cancel.state==State::Stopped);
 Controller quality(cfg);arm(quality,r);quality.audio(3000,true,true,r);quality.audio(3000,false,true,r);quality.audio(3000,true,true,r);assert(quality.state==State::Armed);
 cfg.roles[1]=Role::Right;Controller right(cfg);r[1].cm=30;arm(right,r);run(right,r);assert(right.steeringUs==1500);
 cfg.roles[0]=Role::Left;cfg.roles[1]=Role::Front;Controller swapped(cfg);
 r[0].cm=30;r[1].cm=200;arm(swapped,r);run(swapped,r);assert(swapped.steeringUs==1600);
 cfg=testConfig();cfg.forwardUs=1580;assert(!validConfig(cfg));cfg.car=83;assert(validConfig(cfg));
 cfg.sideGainUsPerCm=std::numeric_limits<float>::quiet_NaN();assert(!validConfig(cfg));
 cfg=testConfig();uint32_t base=0xFFFFF000u;r[0]={true,200,base+3000};r[1]={true,60,base+3000};
 Controller wrap(cfg);arm(wrap,r,base);run(wrap,r,base+3000);wrap.tick(base+8000,r);assert(wrap.reason==Reason::TimeLimit);
 cfg=testConfig();cfg.allowNoEcho=true;r[0]={false,0,3000};r[1]={false,0,3000};
 Controller openRoad(cfg);arm(openRoad,r);run(openRoad,r);assert(openRoad.steeringUs==1550);
 r[0]={true,70,3010};openRoad.tick(3010,r);assert(openRoad.reason==Reason::Obstacle);
 r[0]={false,0,3000};Controller openStale(cfg);arm(openStale,r);run(openStale,r);
 openStale.tick(3401,r);assert(openStale.reason==Reason::FrontRange);
 puts("PASS: configuration, arming, audio reset, steering signs/clamps, obstacle, invalid/stale range, stop latch, timeout and clock wrap.");
}
