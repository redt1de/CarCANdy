#include "app.h"
#include <lvgl.h>
#include <math.h>
#include "config.h"
#include "vehicle_sim.h"
#if GAUGES_DATA_SOURCE == GAUGES_DATA_SOURCE_LIVE
#include "live_data.h"
#endif

#define UPDATE_PERIOD_MS 16
#define SWEEP_MS 1800

static GaugeType current_type;
static GaugeColor current_color = GAUGE_COLOR;
static const GaugeSpec *current_spec;
static uint32_t sweep_start_ms;
static uint32_t last_ms;
static bool sweeping;
static float sweep_peak;
// Highest reading seen per gauge since boot or the last reset, tracked for every
// gauge so each keeps its own peak across runtime switches. NAN = nothing seen yet.
static float peaks[GAUGE_TYPE_COUNT];

static void track_peaks(const float *readings)
{
    for (int t = 0; t < GAUGE_TYPE_COUNT; t++) {
        if (isnan(peaks[t]) || readings[t] > peaks[t]) peaks[t] = readings[t];
    }
}

static void update_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    uint32_t now = lv_tick_get();
#if GAUGES_DATA_SOURCE == GAUGES_DATA_SOURCE_LIVE
    const float *readings = live_data_step(now - last_ms);
#else
    const float *readings = vehicle_sim_step(now - last_ms);
#endif
    last_ms = now;
    track_peaks(readings);

    // Key-on needle sweep to full scale and back, as the stock cluster does.
    if (sweeping) {
        uint32_t elapsed = now - sweep_start_ms;
        if (elapsed < SWEEP_MS) {
            float travel = 0.5f - 0.5f * cosf(2.0f * (float)M_PI * (float)elapsed / SWEEP_MS);
            float value = current_spec->min + (current_spec->max - current_spec->min) * travel;
            // The needle pushes the marker along during the sweep; it drops to the real peak afterwards.
            sweep_peak = LV_MAX(sweep_peak, value);
            gauge_set(value);
            gauge_set_peak(sweep_peak);
            return;
        }
        sweeping = false;
    }

    gauge_set(readings[current_type]);
    gauge_set_peak(isnan(peaks[current_type]) ? readings[current_type] : peaks[current_type]);
}

void app_reset_peak(GaugeType type)
{
    if (type < GAUGE_TYPE_COUNT) peaks[type] = NAN;
}

void app_reset_all_peaks(void)
{
    for (int t = 0; t < GAUGE_TYPE_COUNT; t++) peaks[t] = NAN;
}

void app_set_gauge(GaugeType type)
{
    current_type = type;
    current_spec = &gauge_spec(type);
    gauge_create(lv_screen_active(), *current_spec, gauge_colors(current_color));
    sweep_start_ms = lv_tick_get();
    sweeping = true;
    sweep_peak = current_spec->min;
}

GaugeType app_get_gauge(void)
{
    return current_type;
}

void app_set_color(GaugeColor color)
{
    current_color = color;
    app_set_gauge(current_type);
}

GaugeColor app_get_color(void)
{
    return current_color;
}

void app_init(void)
{
#if GAUGES_DATA_SOURCE == GAUGES_DATA_SOURCE_LIVE
    live_data_init();
#else
    vehicle_sim_reset(0xB0057);
#endif
    app_reset_all_peaks();
    last_ms = lv_tick_get();
    app_set_gauge(GAUGE_TYPE);
    lv_timer_create(update_cb, UPDATE_PERIOD_MS, NULL);
}
