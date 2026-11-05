#pragma once
#include <stdint.h>
namespace remote {
// Pure logic shared by firmware and host tests.
inline int mapPercent(int percent, int center, int negative, int positive) {
  if (percent < -100) percent=-100;
  if (percent > 100) percent=100;
  return center + (percent < 0 ? (center-negative)*percent/100 : (positive-center)*percent/100);
}
class Deadman {
 public:
  void command(uint32_t now) { at=now; active=true; }
  void clear() { active=false; }
  bool expired(uint32_t now, uint32_t timeout) const { return active && uint32_t(now-at)>=timeout; }
 private:
  uint32_t at=0;
  bool active=false;
};
}
