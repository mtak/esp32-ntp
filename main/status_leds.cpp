// SPDX-License-Identifier: Unlicense
#include "status_leds.h"
#include "status_led_state.h"
#include "config_store.h"
#include "gps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#if SOC_LEDC_SUPPORTED
#include "driver/ledc.h"
#endif

static const char* TAG = "STATUS_LEDS";

#if SOC_LEDC_SUPPORTED
static constexpr ledc_mode_t kMode = LEDC_LOW_SPEED_MODE;
static constexpr ledc_timer_t kTimer = LEDC_TIMER_0;
static constexpr ledc_channel_t kLockChannel = LEDC_CHANNEL_0;
static constexpr ledc_channel_t kPpsChannel = LEDC_CHANNEL_1;

struct LedContext {
  GpsDiscipline* gps;
  bool lockEnabled;
  bool ppsEnabled;
  uint32_t lockDuty;
  uint32_t ppsDuty;
  int lockBrightness;
  int ppsBrightness;
};
static LedContext s_led;

static esp_err_t set_duty(ledc_channel_t channel, uint32_t duty) {
  esp_err_t err = ledc_set_duty(kMode, channel, duty);
  if (err == ESP_OK) err = ledc_update_duty(kMode, channel);
  return err;
}

static void led_task(void*) {
  StatusLedState state(s_led.gps->getAcceptedPpsCount());
  for (;;) {
    state.update(s_led.gps->isLocked(), s_led.gps->getAcceptedPpsCount(),
                 esp_timer_get_time(), s_led.lockBrightness, s_led.ppsBrightness);
    if (s_led.lockEnabled && state.lockDuty != s_led.lockDuty) {
      if (set_duty(kLockChannel, state.lockDuty) != ESP_OK) break;
      s_led.lockDuty = state.lockDuty;
    }
    if (s_led.ppsEnabled && state.ppsDuty != s_led.ppsDuty) {
      if (set_duty(kPpsChannel, state.ppsDuty) != ESP_OK) break;
      s_led.ppsDuty = state.ppsDuty;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  ESP_LOGE(TAG, "LEDC update failed; disabling status LEDs");
  if (s_led.lockEnabled) ledc_stop(kMode, kLockChannel, 0);
  if (s_led.ppsEnabled) ledc_stop(kMode, kPpsChannel, 0);
  vTaskDelete(nullptr);
}

static esp_err_t init_channel(ledc_channel_t channel, int gpio) {
  ledc_channel_config_t cfg = {};
  cfg.gpio_num = gpio;
  cfg.speed_mode = kMode;
  cfg.channel = channel;
  cfg.timer_sel = kTimer;
  cfg.duty = 0;
  cfg.hpoint = 0;
  return ledc_channel_config(&cfg);
}
#endif

void status_leds_start(GpsDiscipline* gps) {
  const int lock = cfg_int(CFG_LED_LOCK);
  const int pps = cfg_int(CFG_LED_PPS);
  if (!status_leds_enabled(lock, pps)) return;
  if (!cfg_led_pins_valid()) {
    ESP_LOGE(TAG, "invalid or conflicting LED GPIO; status LEDs disabled");
    return;
  }
#if SOC_LEDC_SUPPORTED
  ledc_timer_config_t timer = {};
  timer.speed_mode = kMode;
  timer.duty_resolution = LEDC_TIMER_10_BIT;
  timer.timer_num = kTimer;
  timer.freq_hz = 1000;
  timer.clk_cfg = LEDC_AUTO_CLK;
  esp_err_t err = ledc_timer_config(&timer);
  bool lockReady = false, ppsReady = false;
  if (err == ESP_OK && lock >= 0) {
    err = init_channel(kLockChannel, lock);
    lockReady = err == ESP_OK;
  }
  if (err == ESP_OK && pps >= 0) {
    err = init_channel(kPpsChannel, pps);
    ppsReady = err == ESP_OK;
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "LEDC initialization failed (%s); status LEDs disabled", esp_err_to_name(err));
    if (lockReady) ledc_stop(kMode, kLockChannel, 0);
    if (ppsReady) ledc_stop(kMode, kPpsChannel, 0);
    return;
  }
  s_led = {gps, lock >= 0, pps >= 0, 0, 0,
           (int)cfg_int(CFG_LED_LOCK_BRIGHTNESS), (int)cfg_int(CFG_LED_PPS_BRIGHTNESS)};
  if (xTaskCreatePinnedToCore(led_task, "status_leds", 3072, nullptr, 2, nullptr, 1) != pdPASS) {
    ESP_LOGE(TAG, "status LED task could not start; status LEDs disabled");
    if (lockReady) ledc_stop(kMode, kLockChannel, 0);
    if (ppsReady) ledc_stop(kMode, kPpsChannel, 0);
    return;
  }
  ESP_LOGI(TAG, "LEDC status LEDs active at 1 kHz (LOCK GPIO%d, PPS GPIO%d)", lock, pps);
#else
  ESP_LOGW(TAG, "this target has no LEDC peripheral; status LEDs disabled");
#endif
}
