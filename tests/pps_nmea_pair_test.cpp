// SPDX-License-Identifier: Unlicense
#include "../components/gps/pps_nmea_pair.h"
#include <assert.h>

int main() {
  // A recurring isolated late RMC is bridged, then the next good label
  // clears the suspicion so another isolated delay may be bridged later.
  auto late = pair_pps_nmea(100, 100, 1000000, -1.0, 0);
  assert(late.action == PpsNmeaAction::UsePreviousPps);
  assert(late.second == 101 && late.streak == 1);
  auto normal = pair_pps_nmea(102, late.second, 1000000, 0.0, late.streak);
  assert(normal.action == PpsNmeaAction::UseNmea && normal.streak == 0);

  // A real, persistent one-second step must not be hidden indefinitely.
  auto second = pair_pps_nmea(101, 101, 1000000, -1.0, late.streak);
  assert(second.action == PpsNmeaAction::Skip && second.streak == 2);
  auto third = pair_pps_nmea(102, 101, 2000000, -1.0, second.streak);
  assert(third.action == PpsNmeaAction::AcceptStep && third.streak == 0);

  // Gaps, bad pulse intervals, and other clock offsets are not bridgeable.
  assert(pair_pps_nmea(100, 100, 2000000, -1.0, 0).action == PpsNmeaAction::Skip);
  assert(pair_pps_nmea(100, 99, 1000000, -1.0, 0).action == PpsNmeaAction::Skip);
  assert(pair_pps_nmea(100, 100, 1000000, 1.0, 0).action == PpsNmeaAction::Skip);
  assert(pair_pps_nmea(100, 100, 1000000, -0.7, 0).action == PpsNmeaAction::Skip);
}
