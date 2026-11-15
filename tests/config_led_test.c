// SPDX-License-Identifier: Unlicense
// Host test of the actual registry, staging, pin checks, and NVS aliases.
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../components/config/config_store.c"

typedef struct { char key[16]; int32_t value; bool used; } slot_t;
static slot_t slots[CFG_COUNT + 4];

const char* esp_err_to_name(esp_err_t err) { (void)err; return "fake"; }
esp_err_t nvs_open(const char* ns, int mode, nvs_handle_t* h) {
  (void)ns; (void)mode; *h = 1; return ESP_OK;
}
static slot_t* find_slot(const char* key) {
  for (size_t i = 0; i < sizeof(slots) / sizeof(slots[0]); ++i)
    if (slots[i].used && strcmp(slots[i].key, key) == 0) return &slots[i];
  return NULL;
}
esp_err_t nvs_get_i32(nvs_handle_t h, const char* key, int32_t* value) {
  (void)h; slot_t* s = find_slot(key); if (!s) return ESP_FAIL;
  *value = s->value; return ESP_OK;
}
esp_err_t nvs_set_i32(nvs_handle_t h, const char* key, int32_t value) {
  (void)h; if (strlen(key) > 15) return ESP_ERR_INVALID_ARG;
  slot_t* s = find_slot(key);
  if (!s) {
    for (size_t i = 0; i < sizeof(slots) / sizeof(slots[0]); ++i)
      if (!slots[i].used) { s = &slots[i]; break; }
  }
  if (!s) return ESP_FAIL;
  strcpy(s->key, key); s->value = value; s->used = true; return ESP_OK;
}
esp_err_t nvs_get_str(nvs_handle_t h, const char* key, char* out, size_t* len) {
  (void)h; (void)key; (void)out; (void)len; return ESP_FAIL;
}
esp_err_t nvs_set_str(nvs_handle_t h, const char* key, const char* value) {
  (void)h; (void)key; (void)value; return ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t h, const char* key, uint8_t* value) {
  (void)h; (void)key; (void)value; return ESP_FAIL;
}
esp_err_t nvs_set_u8(nvs_handle_t h, const char* key, uint8_t value) {
  (void)h; (void)key; (void)value; return ESP_OK;
}
esp_err_t nvs_erase_all(nvs_handle_t h) { (void)h; memset(slots, 0, sizeof(slots)); return ESP_OK; }
esp_err_t nvs_commit(nvs_handle_t h) { (void)h; return ESP_OK; }
void nvs_close(nvs_handle_t h) { (void)h; }

int main(void) {
  cfg_init(false);
  assert(cfg_int(CFG_LED_LOCK) == -1 && cfg_int(CFG_LED_PPS) == -1);
  assert(cfg_int(CFG_LED_LOCK_BRIGHTNESS) == 100);
  assert(cfg_int(CFG_LED_PPS_BRIGHTNESS) == 100);
  assert(cfg_lookup("led.lock_brightness") == CFG_LED_LOCK_BRIGHTNESS);
  assert(!cfg_stage(CFG_LED_LOCK_BRIGHTNESS, "-1"));
  assert(!cfg_stage(CFG_LED_PPS_BRIGHTNESS, "101"));
  assert(!cfg_stage(CFG_LED_PPS_BRIGHTNESS, "1.5"));
  assert(cfg_stage(CFG_LED_LOCK_BRIGHTNESS, "0"));
  assert(cfg_stage(CFG_LED_PPS_BRIGHTNESS, "1"));
  assert(cfg_stage(CFG_DISP_EN, "0")); // default GPIO18/23 belong to display
  assert(cfg_stage(CFG_LED_LOCK, "18"));
  assert(cfg_stage(CFG_LED_PPS, "23"));
  assert(cfg_led_pins_valid());
  assert(cfg_commit() == ESP_OK);
  assert(find_slot("led.lock_brt")->value == 0);
  assert(find_slot("led.pps_brt")->value == 1);
  cfg_init(false);
  assert(cfg_int(CFG_LED_LOCK) == 18 && cfg_int(CFG_LED_PPS) == 23);
  assert(cfg_int(CFG_LED_LOCK_BRIGHTNESS) == 0);
  assert(cfg_int(CFG_LED_PPS_BRIGHTNESS) == 1);
  assert(cfg_stage(CFG_LED_LOCK_BRIGHTNESS, "50"));
  assert(cfg_stage(CFG_LED_PPS_BRIGHTNESS, "100"));
  assert(cfg_commit() == ESP_OK);
  cfg_init(false);
  assert(cfg_int(CFG_LED_LOCK_BRIGHTNESS) == 50);
  assert(cfg_int(CFG_LED_PPS_BRIGHTNESS) == 100);
  assert(cfg_stage(CFG_LED_PPS, "18"));
  assert(!cfg_led_pins_valid() && cfg_commit() == ESP_ERR_INVALID_ARG);
  assert(cfg_stage(CFG_LED_PPS, "6")); // flash pin
  assert(!cfg_led_pins_valid());
  assert(cfg_stage(CFG_LED_PPS, "19")); // PPS input
  assert(!cfg_led_pins_valid());
  assert(cfg_stage(CFG_LED_PPS, "34") == false); // input only
  assert(cfg_stage(CFG_LED_PPS, "23"));
  assert(cfg_stage(CFG_DISP_EN, "1"));
  assert(!cfg_led_pins_valid()); // display owns 18 and 23 again
}
