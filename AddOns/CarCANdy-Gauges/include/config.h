// Build-time project settings.
#pragma once

// Which gauge the display shows. One of:
//   GAUGE_BOOST    boost / vacuum (PSI)
//   GAUGE_AFR      air/fuel ratio
//   GAUGE_COOLANT  coolant temperature (°F)
//   GAUGE_TRANS    transmission temperature (°F)
//   GAUGE_RPM      engine speed
//   GAUGE_SPEED    vehicle speed (MPH)
//   GAUGE_VOLTAGE  battery / charging voltage
//   GAUGE_OIL_PRESSURE  engine oil pressure (PSI)
// Can also be set per build with -D GAUGE_TYPE=... in platformio.ini.
#ifndef GAUGE_TYPE
#define GAUGE_TYPE GAUGE_OIL_PRESSURE
#endif

// Colour of the markings, numerals and needle. One of:
//   GAUGE_COLOR_RED, GAUGE_COLOR_ORANGE, GAUGE_COLOR_GREEN, GAUGE_COLOR_BLUE,
//   GAUGE_COLOR_ICE_BLUE, GAUGE_COLOR_PURPLE, GAUGE_COLOR_WHITE
//   GAUGE_COLOR_CUSTOM  uses GAUGE_COLOR_CUSTOM_RGB below
#ifndef GAUGE_COLOR
#define GAUGE_COLOR GAUGE_COLOR_RED
#endif

// 0xRRGGBB, only used with GAUGE_COLOR_CUSTOM.
#ifndef GAUGE_COLOR_CUSTOM_RGB
#define GAUGE_COLOR_CUSTOM_RGB 0xffb000
#endif
