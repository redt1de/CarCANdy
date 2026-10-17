#pragma once
#include <stdint.h>
#include "gauge.h"

// Fake car: loops through idle, cruise, full-throttle pulls with gear shifts and
// lift-off, and derives a plausible reading for every gauge from that one drive
// cycle. Stands in for real sensors.
void vehicle_sim_reset(uint32_t seed);
// Advance the model by dt_ms. Returns the current readings indexed by GaugeType,
// in each gauge's own units.
const float *vehicle_sim_step(uint32_t dt_ms);
