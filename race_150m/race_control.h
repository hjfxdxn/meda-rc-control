#pragma once
#include <stdint.h>

namespace race {
enum class Role { Unassigned, Front, Left, Right };
enum class State { WaitingConfiguration, Settling, WaitingClear, Armed, Running, Stopped };
enum class Reason { None, UserStop, Configuration, FrontRange, SideRange, Obstacle, TimeLimit, OutputFault };
struct Config {
  bool confirmed = false;
  bool allowNoEcho = false; // Optional open-road policy; stale measurements still stop.
  int car = 0;
  int neutralUs = 0, forwardUs = 0;
  int centerUs = 0, leftUs = 0, rightUs = 0;
  Role roles[2] = {Role::Unassigned, Role::Unassigned};
  unsigned matchWindows = 0;
  float stopCm = 0, sideTargetCm = 0, sideGainUsPerCm = 0;
  int maxCorrectionUs = 0;
  uint32_t maxRunMs = 0;
  uint32_t rangeMaxAgeMs = 400;
};
struct Range {
  bool valid = false;
  float cm = 0;
  uint32_t atMs = 0;
};
inline bool fresh(const Range& r, uint32_t now, uint32_t age) {
  return r.valid && r.cm >= 2 && r.cm <= 400 && uint32_t(now-r.atMs) <= age;
}
inline bool validConfig(const Config& c) {
  const bool car = c.car == 81 || c.car == 82 || c.car == 83;
  const bool direction = c.car == 83 ? c.forwardUs > c.neutralUs : c.forwardUs < c.neutralUs;
  const bool roles = (c.roles[0] == Role::Front && (c.roles[1] == Role::Left || c.roles[1] == Role::Right)) ||
                     (c.roles[1] == Role::Front && (c.roles[0] == Role::Left || c.roles[0] == Role::Right));
  return c.confirmed && car && direction && roles && c.neutralUs >= 1000 && c.neutralUs <= 2000 &&
    c.forwardUs >= 1000 && c.forwardUs <= 2000 && c.leftUs >= 900 && c.rightUs <= 2100 &&
    c.leftUs < c.centerUs && c.centerUs < c.rightUs && c.maxCorrectionUs > 0 &&
    c.maxCorrectionUs <= c.centerUs-c.leftUs && c.maxCorrectionUs <= c.rightUs-c.centerUs &&
    c.matchWindows > 0 && c.matchWindows <= 20 && c.stopCm >= 2 && c.stopCm < 400 &&
    c.sideTargetCm >= 2 && c.sideTargetCm <= 400 && c.sideGainUsPerCm > 0 && c.sideGainUsPerCm <= 20 &&
    c.maxRunMs > 0 && c.maxRunMs <= 300000 && c.rangeMaxAgeMs >= 150 && c.rangeMaxAgeMs <= 1000;
}
class Controller {
 public:
  explicit Controller(const Config& c) : cfg(c) {}
  void begin(uint32_t now) {
    since = now;
    state = validConfig(cfg) ? State::Settling : State::WaitingConfiguration;
  }
  void stop(Reason why) { state = State::Stopped; reason = why; }
  void tick(uint32_t now, const Range (&ranges)[2]) {
    if (state == State::Settling && uint32_t(now-since) >= 3000) state = State::WaitingClear;
    if (state != State::Running) return;
    if (uint32_t(now-since) >= cfg.maxRunMs) { stop(Reason::TimeLimit); return; }
    if (!rangesReady(now, ranges)) return;
    const int side = cfg.roles[0] == Role::Front ? 1 : 0;
    if (!fresh(ranges[side], now, cfg.rangeMaxAgeMs)) { steeringUs = cfg.centerUs; return; }
    // Positive pulse delta steers right on the recorded vehicles.
    float delta = (cfg.sideTargetCm-ranges[side].cm)*cfg.sideGainUsPerCm;
    if (cfg.roles[side] == Role::Right) delta = -delta;
    if (delta > cfg.maxCorrectionUs) delta = float(cfg.maxCorrectionUs);
    if (delta < -cfg.maxCorrectionUs) delta = float(-cfg.maxCorrectionUs);
    steeringUs = cfg.centerUs + int(delta);
  }
  void audio(uint32_t now, bool valid, bool match, const Range (&ranges)[2]) {
    if (state == State::WaitingClear) {
      if (valid && !match) { if (++clear >= 3) { state = State::Armed; matches = 0; } }
      else clear = 0;
    } else if (state == State::Armed) {
      if (valid && match) ++matches; else matches = 0;
      if (matches >= cfg.matchWindows) {
        if (!rangesReady(now, ranges)) return;
        state = State::Running; since = now; steeringUs = cfg.centerUs;
        tick(now, ranges);
      }
    }
  }
  State state = State::WaitingConfiguration;
  Reason reason = Reason::None;
  int steeringUs = 0;
 private:
  bool rangesReady(uint32_t now, const Range (&ranges)[2]) {
    for (int i=0;i<2;++i) {
      if (uint32_t(now-ranges[i].atMs) > cfg.rangeMaxAgeMs ||
          (!fresh(ranges[i], now, cfg.rangeMaxAgeMs) && !cfg.allowNoEcho)) {
        stop(cfg.roles[i] == Role::Front ? Reason::FrontRange : Reason::SideRange); return false;
      }
      if (cfg.roles[i] == Role::Front && fresh(ranges[i], now, cfg.rangeMaxAgeMs) && ranges[i].cm <= cfg.stopCm) { stop(Reason::Obstacle); return false; }
    }
    return true;
  }
  Config cfg;
  uint32_t since = 0;
  unsigned clear = 0, matches = 0;
};
}
