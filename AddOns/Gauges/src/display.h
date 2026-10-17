#pragma once
#include <lvgl.h>

// Brings up the Waveshare ESP32-S3-Touch-LCD-2.8C panel and registers it with
// LVGL. Call after lv_init().
lv_display_t *display_init(void);
// 0..100 %
void display_set_backlight(uint8_t percent);
