// SPDX-License-Identifier: Unlicense
#pragma once

class GpsDiscipline;

// Starts only when a configured LED can be driven. Failure is nonfatal.
void status_leds_start(GpsDiscipline* gps);
