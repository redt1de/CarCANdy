#pragma once
#include "gauge.h"

// Shows the gauge and colour scheme selected in config.h and starts feeding it simulated readings.
void app_init(void);
// Switch to another gauge at runtime (e.g. from a button handler).
void app_set_gauge(GaugeType type);
GaugeType app_get_gauge(void);
// Switch colour scheme at runtime.
void app_set_color(GaugeColor color);
GaugeColor app_get_color(void);
// Clear the peak marker for one gauge, e.g. from a reset button:
// app_reset_peak(app_get_gauge()) resets the one on screen.
void app_reset_peak(GaugeType type);
void app_reset_all_peaks(void);
