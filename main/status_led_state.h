// SPDX-License-Identifier: Unlicense
#pragma once
#include <stdint.h>

static inline bool status_leds_enabled(int lockGpio, int ppsGpio) {
  return lockGpio >= 0 || ppsGpio >= 0;
}

// LEDC's 10-bit duty register accepts 0..1024. The upper endpoint holds the
// output high, so 100% is genuinely full on rather than a 1023/1024 pulse.
static inline uint32_t status_led_duty(int percent) {
  if (percent <= 0) return 0;
  if (percent >= 100) return 1024;
  return (uint32_t)(percent * 1024 + 50) / 100;
}

struct StatusLedState {
  uint32_t seenPps;
  int64_t ppsUntilUs;
  uint32_t lockDuty;
  uint32_t ppsDuty;

  explicit StatusLedState(uint32_t initialPps)
    : seenPps(initialPps), ppsUntilUs(0), lockDuty(0), ppsDuty(0) {}

  void update(bool locked, uint32_t acceptedPps, int64_t nowUs,
              int lockBrightness, int ppsBrightness) {
    lockDuty = locked ? status_led_duty(lockBrightness) : 0;
    if (acceptedPps != seenPps) {
      seenPps = acceptedPps;
      ppsUntilUs = ppsBrightness > 0 ? nowUs + 75000 : 0;
    }
    ppsDuty = nowUs < ppsUntilUs ? status_led_duty(ppsBrightness) : 0;
  }
};
