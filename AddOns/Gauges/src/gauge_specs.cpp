#include "gauge.h"
#include "config.h"

#define DEG "\xC2\xB0"

static const char *const boost_labels[] = {"15", "10", "5", "0", "5", "10", "15", "20", "25", NULL};
static const char *const afr_labels[] = {"10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", NULL};
static const char *const temp_labels[] = {"100", "120", "140", "160", "180", "200", "220", "240", "260", NULL};
static const char *const rpm_labels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", NULL};
static const char *const speed_labels[] = {"0", "20", "40", "60", "80", "100", "120", "140", "160", NULL};
static const char *const volt_labels[] = {"8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", NULL};
static const char *const oil_labels[] = {"0", "10", "20", "30", "40", "50", "60", "70", "80", NULL};

// Indexed by GaugeType.
static const GaugeSpec specs[GAUGE_TYPE_COUNT] = {
    // name      caption                   unit       min   max  ticks maj labels          dim  dec step  band  peak   note
    {"BOOST",   "BOOST PRESSURE",         "PSI",     -15,   25, 41, 5, boost_labels,      0, 1, 0.1f, 0.2f, true,  "VAC", -104, 52},
    {"AFR",     "AIR / FUEL RATIO",       "AFR",      10,   20, 41, 4, afr_labels,       10, 1, 0.1f, 0.1f, false, NULL, 0, 0},
    {"COOLANT", "COOLANT TEMP",           DEG "F",   100,  260, 33, 4, temp_labels,     160, 0, 1,    1,    false, NULL, 0, 0},
    {"RPM",     "REVOLUTIONS PER MINUTE", "RPM",       0, 8000, 33, 4, rpm_labels,        0, 0, 10,   20,   false, "x1000", 0, 68},
    {"TRANS",   "TRANS TEMP",             DEG "F",   100,  260, 33, 4, temp_labels,     140, 0, 1,    1,    false, NULL, 0, 0},
    {"SPEED",   "GROUND SPEED",           "MPH",       0,  160, 33, 4, speed_labels,      0, 0, 1,    1,    false, NULL, 0, 0},
    {"VOLTAGE", "BATTERY VOLTAGE",        "VOLTS",     8,   18, 41, 4, volt_labels,      12, 1, 0.1f, 0.1f, false, NULL, 0, 0},
    {"OIL",     "OIL PRESSURE",           "PSI",       0,   80, 33, 4, oil_labels,        0, 0, 1,    1,    false, NULL, 0, 0},
};

const GaugeSpec &gauge_spec(GaugeType type)
{
    return specs[type < GAUGE_TYPE_COUNT ? type : GAUGE_BOOST];
}

// Custom scheme: the needle is the chosen colour lightened 30% toward white.
#define LIGHTEN_CH(rgb, shift) ((((rgb) >> (shift)) & 0xff) + (255 - (((rgb) >> (shift)) & 0xff)) * 3 / 10)
#define LIGHTEN(rgb) ((uint32_t)(LIGHTEN_CH(rgb, 16) << 16 | LIGHTEN_CH(rgb, 8) << 8 | LIGHTEN_CH(rgb, 0)))

// Indexed by GaugeColor.
static const GaugeColors color_schemes[GAUGE_COLOR_COUNT] = {
    // name       markings  needle
    {"RED",      0xff2413, 0xff5a2a},
    {"ORANGE",   0xff6a00, 0xff9a3c},
    {"GREEN",    0x19e63c, 0x7dff8a},
    {"BLUE",     0x2a5cff, 0x7a9cff},
    {"ICE_BLUE", 0x6fd6ff, 0xc4efff},
    {"PURPLE",   0xa030ff, 0xc98aff},
    {"WHITE",    0xf2f2f2, 0xffffff},
    {"CUSTOM",   GAUGE_COLOR_CUSTOM_RGB, LIGHTEN(GAUGE_COLOR_CUSTOM_RGB)},
};

const GaugeColors &gauge_colors(GaugeColor color)
{
    return color_schemes[color < GAUGE_COLOR_COUNT ? color : GAUGE_COLOR_RED];
}
