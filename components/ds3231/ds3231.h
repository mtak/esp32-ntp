#pragma once
// SPDX-License-Identifier: Unlicense
#include <time.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

/* optional battery RTC + TCXO */
class Ds3231 {
public:
  esp_err_t begin(int sdaPin, int sclPin);
  bool present() const { return dev != nullptr; }
  // UTC; false if OSF set
  bool readTime(time_t& out);
  // write restarts divider chain
  esp_err_t setTime(time_t t);
  // die temp, 0.25C steps
  bool readTempC(float& out);
  // Keep the battery oscillator running; enable 32k output only when wired.
  esp_err_t configure32kOutput(bool enabled);

private:
  esp_err_t rd(uint8_t reg, uint8_t* buf, size_t n);
  esp_err_t wr(uint8_t reg, const uint8_t* buf, size_t n);
  i2c_master_bus_handle_t bus = nullptr;
  i2c_master_dev_handle_t dev = nullptr;
};
