#pragma once
#include "race_control.h"

// Standalone sound-start and manual output are enabled for car 82 trials.
// Do not copy steering center
// into ESC neutral. Populate only after vehicle, rules and mounting are confirmed.
constexpr bool ENABLE_MOTOR_OUTPUT = true;
constexpr bool ENABLE_WIFI = true; // Set false to isolate USB/sensor testing.
constexpr int REVERSE_TEST_PULSE_US = 1580; // Recorded car 82 reverse onset, not full reverse.
inline race::Config makeRaceConfig() {
  race::Config c;
  c.car = 82; // Vehicle selection is explicit, independent of board MAC address.
  c.neutralUs = 1530; // Legacy bench setting, still needs car 82 neutral verification.
  c.forwardUs = 1200; // User-selected default forward trial pulse, not a measured speed.
  c.centerUs = 1550; c.leftUs = 1150; c.rightUs = 1950; // Recorded car 82 limits.
  c.roles[0] = race::Role::Left;  // User confirmed J4: TRIG D9 / ECHO D11.
  c.roles[1] = race::Role::Front; // User confirmed J7: TRIG D8 / ECHO D12.
  c.matchWindows = 2; // Existing sound-test behavior; course timing still pending.
  // Development defaults, not measured stopping-distance or race certification.
  c.stopCm = 80;
  c.sideTargetCm = 60; c.sideGainUsPerCm = 1; c.maxCorrectionUs = 50;
  c.maxRunMs = 180000; // Watchdog only; no distance measurement.
  c.rangeMaxAgeMs = 1000; // Includes stationary audio/network processing gaps.
  c.allowNoEcho = true; // Open road: no echo is unknown, not a proven clear path.
  c.confirmed = true; // User-authorized standalone sound-start trial.
  return c;
}
