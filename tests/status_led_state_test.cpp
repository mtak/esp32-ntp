// SPDX-License-Identifier: Unlicense
#include "../main/status_led_state.h"
#include <assert.h>

int main() {
  assert(status_led_duty(0) == 0);
  assert(status_led_duty(1) == 10);
  assert(status_led_duty(50) == 512);
  assert(status_led_duty(100) == 1024);
  assert(!status_leds_enabled(-1, -1));
  assert(status_leds_enabled(18, -1));
  assert(status_leds_enabled(-1, 23));
  assert(status_leds_enabled(18, 23));

  StatusLedState led(4);
  led.update(false, 4, 1000000, 50, 1);
  assert(led.lockDuty == 0 && led.ppsDuty == 0);
  led.update(true, 4, 1010000, 50, 1);
  assert(led.lockDuty == 512 && led.ppsDuty == 0);
  led.update(true, 5, 1020000, 50, 1);
  assert(led.lockDuty == 512 && led.ppsDuty == 10);
  led.update(true, 5, 1094999, 50, 1);
  assert(led.ppsDuty == 10);
  led.update(true, 5, 1095000, 50, 1);
  assert(led.ppsDuty == 0);
  led.update(false, 6, 1100000, 100, 50);
  assert(led.lockDuty == 0 && led.ppsDuty == 512);
  led.update(true, 7, 1200000, 100, 100);
  assert(led.lockDuty == 1024 && led.ppsDuty == 1024);
  led.update(true, 7, 1275000, 100, 100);
  assert(led.ppsDuty == 0); // no repeated flash after PPS stops
  led.update(true, 8, 1300000, 0, 0);
  assert(led.lockDuty == 0 && led.ppsDuty == 0);
  led.update(true, 8, 1400000, 0, 0);
  assert(led.ppsDuty == 0);
}
