#pragma once
#include <stdint.h>
#include <math.h>
constexpr float SOUND_RMS_THRESHOLD = 50.0f;
struct SimpleRms {
  int64_t sum = 0;
  uint64_t squares = 0;
  uint32_t count = 0;
  void add(int sample) {
    int32_t centered = sample - 2048;
    sum += centered;
    squares += int64_t(centered) * centered;
    ++count;
  }
  float value() const {
    if (!count) return 0;
    double mean = double(sum) / count;
    double variance = double(squares) / count - mean * mean;
    return float(sqrt(variance > 0 ? variance : 0));
  }
};
