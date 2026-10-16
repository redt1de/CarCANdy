#pragma once
#include <lvgl.h>

enum GaugeType {
    GAUGE_BOOST,
    GAUGE_AFR,
    GAUGE_COOLANT,
    GAUGE_RPM,
    GAUGE_TRANS,
    GAUGE_SPEED,
    GAUGE_VOLTAGE,
    GAUGE_OIL_PRESSURE,
    GAUGE_TYPE_COUNT,
};

enum GaugeColor {
    GAUGE_COLOR_RED,
    GAUGE_COLOR_ORANGE,
    GAUGE_COLOR_GREEN,
    GAUGE_COLOR_BLUE,
    GAUGE_COLOR_ICE_BLUE,
    GAUGE_COLOR_PURPLE,
    GAUGE_COLOR_WHITE,
    GAUGE_COLOR_CUSTOM,
    GAUGE_COLOR_COUNT,
};

// A colour scheme. The dimmed markings, needle glow and hub ring are derived from these.
struct GaugeColors {
    const char *name;   // short id, e.g. "RED"
    uint32_t markings;  // ticks, numerals and text, 0xRRGGBB
    uint32_t needle;    // needle core, a hotter shade of the same hue
};

// Everything that differs between gauges. The face itself (colours, sweep,
// needle, layout) is shared and lives in gauge.cpp; definitions are in gauge_specs.cpp.
struct GaugeSpec {
    const char *name;           // short id, e.g. "BOOST"
    const char *caption;        // small text above the hub
    const char *unit;           // under the digital readout
    float min, max;             // dial range
    uint8_t tick_count;         // all ticks, including both ends
    uint8_t major_every;        // every Nth tick is a major (labelled) one
    const char *const *labels;  // major tick labels, NULL terminated
    float dim_below;            // markings below this value are dimmed; <= min for none
    uint8_t decimals;           // digits after the point in the readout
    float readout_step;         // readout is rounded to a multiple of this
    float readout_deadband;     // readout only changes once the value moves this far
    bool show_peak;             // also print the peak value under the readout
    const char *note;           // optional extra small caption, or NULL
    int16_t note_x, note_y;     // its offset from the dial centre
};

const GaugeSpec &gauge_spec(GaugeType type);
const GaugeColors &gauge_colors(GaugeColor color);

// Replaces whatever is on `parent` with the given gauge.
void gauge_create(lv_obj_t *parent, const GaugeSpec &spec, const GaugeColors &colors);
void gauge_set(float value);
// Moves the peak marker (and printed peak, if the gauge has one) to `value`.
void gauge_set_peak(float value);
