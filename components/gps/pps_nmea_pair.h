// SPDX-License-Identifier: Unlicense
#pragma once

#include <stdint.h>
#include <math.h>

enum class PpsNmeaAction { UseNmea, UsePreviousPps, Skip, AcceptStep };

struct PpsNmeaPair {
  int64_t second;
  uint8_t streak;
  PpsNmeaAction action;
};

// A single late RMC may describe the PPS already accepted on the previous
// cycle. Bridge only that exact case, with a sane measured PPS interval. A
// continuing disagreement goes through the normal persistent-step guard.
static inline PpsNmeaPair pair_pps_nmea(int64_t nmeaSecond,
                                         int64_t previousSecond,
                                         int64_t pulseIntervalUs,
                                         double clockDifferenceSec,
                                         uint8_t streak) {
  if (fabs(clockDifferenceSec) <= 0.5) {
    return {nmeaSecond, 0, PpsNmeaAction::UseNmea};
  }
  if (streak == 0 && previousSecond > 0 && nmeaSecond == previousSecond &&
      pulseIntervalUs >= 900000 && pulseIntervalUs <= 1100000 &&
      fabs(clockDifferenceSec + 1.0) < 0.1) {
    return {previousSecond + 1, 1, PpsNmeaAction::UsePreviousPps};
  }
  if (streak < 2) {
    return {nmeaSecond, (uint8_t)(streak + 1), PpsNmeaAction::Skip};
  }
  return {nmeaSecond, 0, PpsNmeaAction::AcceptStep};
}
