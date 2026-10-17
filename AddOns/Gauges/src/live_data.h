#pragma once
#include <stdint.h>
#include "gauge.h"

// Parses CarCANdy's GaugesPlugin broadcast (see
// Firmware/Teensy/src/plugins/GaugesPlugin.h for the wire format) into the
// same per-GaugeType readings array vehicle_sim_step() returns, so app.cpp
// can swap between the two with one #define -- see GAUGES_DATA_SOURCE in
// config.h.
void live_data_init(void);
// Reads whatever bytes have arrived since the last call and parses any
// complete lines. dt_ms is unused (kept only so this matches
// vehicle_sim_step()'s signature). A signal CarCANdy hasn't sent a real
// value for yet (still "nan" on the wire, e.g. a gauge whose CAN ID isn't
// implemented in GaugesPlugin yet) holds at its gauge's spec minimum.
const float *live_data_step(uint32_t dt_ms);
