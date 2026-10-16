#include <Arduino.h>
#include <lvgl.h>
#include "app.h"
#include "display.h"

static uint32_t tick_cb(void)
{
    return millis();
}

void setup()
{
    Serial.begin(115200);

    lv_init();
    lv_tick_set_cb(tick_cb);
    display_init();
    app_init();
}

void loop()
{
    uint32_t wait_ms = lv_timer_handler();
    delay(wait_ms < 5 ? wait_ms : 5);
}
