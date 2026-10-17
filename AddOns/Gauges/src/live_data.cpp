#include "live_data.h"
#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// GPIO44 is free on this board: with ARDUINO_USB_MODE/ARDUINO_USB_CDC_ON_BOOT
// set (see platformio.ini), Serial is the USB-CDC console rather than
// UART0, so pins 43/44 (U0TXD/U0RXD) aren't claimed by anything else.
// CarCANdy's GaugesPlugin only transmits (one-wire broadcast to all 3
// gauges), so only RX is wired up here.
#define LIVE_DATA_RX_PIN 44
#define LIVE_DATA_BAUD 115200

static float readings[GAUGE_TYPE_COUNT];
static char line_buf[160];
static size_t line_len;

void live_data_init(void)
{
    for (int t = 0; t < GAUGE_TYPE_COUNT; t++) readings[t] = gauge_spec((GaugeType)t).min;
    Serial1.begin(LIVE_DATA_BAUD, SERIAL_8N1, LIVE_DATA_RX_PIN, -1);
    line_len = 0;
}

// Parses one "NAME:VALUE,NAME:VALUE,..." line (see GaugesPlugin.h for the
// exact format) into readings[], matching NAME against each GaugeType's
// own GaugeSpec.name. Updates are per-field, not per-line: "nan" (or
// anything else strtof() can't parse) leaves that one field untouched, so
// a gauge GaugesPlugin hasn't implemented yet just never advances past its
// initial spec minimum instead of freezing the whole line's updates.
static void parse_line(char *line)
{
    char *tok = line;
    while (tok && *tok) {
        char *comma = strchr(tok, ',');
        if (comma) *comma = '\0';

        char *colon = strchr(tok, ':');
        if (colon) {
            *colon = '\0';
            char *end;
            float value = strtof(colon + 1, &end);
            if (end != colon + 1 && !isnan(value)) {
                for (int t = 0; t < GAUGE_TYPE_COUNT; t++) {
                    if (strcmp(tok, gauge_spec((GaugeType)t).name) == 0) {
                        readings[t] = value;
                        break;
                    }
                }
            }
        }
        tok = comma ? comma + 1 : NULL;
    }
}

const float *live_data_step(uint32_t dt_ms)
{
    (void)dt_ms;
    while (Serial1.available()) {
        char c = (char)Serial1.read();
        if (c == '\n') {
            line_buf[line_len] = '\0';
            parse_line(line_buf);
            line_len = 0;
        } else if (c != '\r' && line_len < sizeof(line_buf) - 1) {
            line_buf[line_len++] = c;
        }
    }
    return readings;
}
