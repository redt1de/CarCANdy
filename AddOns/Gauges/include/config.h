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

// Where gauge readings come from. One of:
//   GAUGES_DATA_SOURCE_SIM   vehicle_sim.cpp's fake drive cycle (default --
//                            lets the visual style be tuned with no wiring)
//   GAUGES_DATA_SOURCE_LIVE  live_data.cpp, parsed from CarCANdy's
//                            GaugesPlugin broadcast over UART (GPIO44 RX,
//                            see live_data.cpp)
// Can also be set per build with -D GAUGES_DATA_SOURCE=... in
// platformio.ini. Flip this one line to switch between developing the
// visuals against the simulator and showing real data -- app.cpp is the
// only other place that cares which one is active.
#define GAUGES_DATA_SOURCE_SIM 0
#define GAUGES_DATA_SOURCE_LIVE 1
#ifndef GAUGES_DATA_SOURCE
#define GAUGES_DATA_SOURCE GAUGES_DATA_SOURCE_SIM
#endif
