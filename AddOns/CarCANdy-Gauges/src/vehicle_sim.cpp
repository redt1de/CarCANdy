#include "vehicle_sim.h"
#include <math.h>

enum Phase { IDLE, CRUISE, PULL, SHIFT, LIFT };

#define TOP_GEAR 6
#define IDLE_RPM 780.0f
#define REDLINE_RPM 6900.0f

// Indexed by gear, 1..TOP_GEAR.
static const float rpm_per_mph[TOP_GEAR + 1] = {0, 150, 98, 70, 53, 42, 27};
static const float pull_accel_mph_s[TOP_GEAR + 1] = {0, 16, 11, 7.5f, 5, 3.5f, 2};

static uint32_t rng_state = 1;
static Phase phase;
static int32_t phase_ms_left;
static int shifts_left;
static int gear;
static uint32_t clock_ms;

static float boost, boost_target;
static float speed, cruise_speed;
static float rpm;
static float afr;
static float coolant;
static float trans;
static float volts;
static float oil;

static float readings[GAUGE_TYPE_COUNT];

static float rnd(float lo, float hi)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return lo + (hi - lo) * (float)(rng_state & 0xffffff) / (float)0xffffff;
}

// First-order lag: how far to move toward a target over dt_ms.
static float lag(float dt_ms, float tau_ms)
{
    return 1.0f - expf(-dt_ms / tau_ms);
}

static void enter(Phase next)
{
    Phase prev = phase;
    phase = next;
    switch (next) {
    case IDLE:
        boost_target = -10.5f;
        phase_ms_left = (int32_t)rnd(1500, 3000);
        break;
    case CRUISE:
        boost_target = rnd(-7.0f, -2.0f);
        cruise_speed = rnd(30, 75);
        phase_ms_left = (int32_t)rnd(2000, 4000);
        break;
    case PULL:
        boost_target = rnd(15.0f, 21.5f);
        phase_ms_left = (int32_t)rnd(1800, 3200);
        if (prev == SHIFT) {
            if (gear < TOP_GEAR) gear++;
        } else {
            // Kick down to the lowest gear that leaves room to rev.
            gear = 1;
            while (gear < TOP_GEAR && speed * rpm_per_mph[gear] > 4200) gear++;
        }
        break;
    case SHIFT:
        boost_target = -5.0f;
        phase_ms_left = 260;
        break;
    case LIFT:
        boost_target = -13.0f;
        phase_ms_left = (int32_t)rnd(700, 1200);
        break;
    }
}

static void next_phase(void)
{
    switch (phase) {
    case IDLE:
        enter(CRUISE);
        break;
    case CRUISE:
        if (rnd(0, 1) < 0.75f) {
            shifts_left = (int)rnd(0, 3.99f);
            enter(PULL);
        } else {
            enter(LIFT);
        }
        break;
    case PULL:
        if (shifts_left > 0 && gear < TOP_GEAR) {
            shifts_left--;
            enter(SHIFT);
        } else {
            enter(LIFT);
        }
        break;
    case SHIFT:
        enter(PULL);
        break;
    case LIFT:
        enter(rnd(0, 1) < 0.5f ? IDLE : CRUISE);
        break;
    }
}

void vehicle_sim_reset(uint32_t seed)
{
    rng_state = seed ? seed : 1;
    clock_ms = 0;
    shifts_left = 0;
    gear = 1;
    boost = -10.5f;
    speed = 0;
    rpm = IDLE_RPM;
    afr = 14.7f;
    coolant = 120;
    trans = 105;
    volts = 14.1f;
    oil = 45;
    enter(IDLE);
}

const float *vehicle_sim_step(uint32_t dt_ms)
{
    float dt = (float)dt_ms;
    clock_ms += dt_ms;
    phase_ms_left -= (int32_t)dt_ms;
    if (phase_ms_left <= 0 || (phase == PULL && rpm >= REDLINE_RPM)) next_phase();

    // Vacuum follows the throttle almost instantly; positive pressure has to wait
    // for the turbo to spool, and dumps quickly when the throttle closes.
    float tau_ms;
    if (boost_target < boost) tau_ms = 70.0f;
    else if (boost < 0.0f) tau_ms = 110.0f;
    else tau_ms = 420.0f;
    boost += (boost_target - boost) * lag(dt, tau_ms);

    float dt_s = dt / 1000.0f;
    switch (phase) {
    case PULL:
        speed += pull_accel_mph_s[gear] * dt_s;
        break;
    case CRUISE: {
        float step = 4.0f * dt_s;
        speed += LV_CLAMP(-step, cruise_speed - speed, step);
        break;
    }
    case LIFT:
        speed -= 5.0f * dt_s;
        break;
    case IDLE:
        speed -= 12.0f * dt_s;
        break;
    case SHIFT:
        break;
    }
    speed = LV_CLAMP(0.0f, speed, 158.0f);

    // Off the throttle the box picks the tallest gear that doesn't lug the engine.
    if (phase == CRUISE || phase == IDLE) {
        gear = TOP_GEAR;
        while (gear > 1 && speed * rpm_per_mph[gear] < 1400) gear--;
    }
    float rpm_target = LV_MAX(IDLE_RPM, speed * rpm_per_mph[gear]);
    rpm += (rpm_target - rpm) * lag(dt, 90.0f);

    // Stoichiometric off boost, enriched under it, and fuel cut on lift-off.
    float afr_target;
    if (phase == LIFT) afr_target = 19.6f;
    else afr_target = 14.7f - 3.2f * LV_CLAMP(0.0f, boost / 12.0f, 1.0f);
    afr += (afr_target - afr) * lag(dt, 120.0f);

    // Temperatures warm up far faster than a real car so the demo shows them moving.
    float load = LV_MAX(0.0f, boost);
    float thermostat = 4.0f * sinf((float)clock_ms * 2.0f * (float)M_PI / 20000.0f);
    coolant += (194.0f + thermostat + 0.5f * load - coolant) * lag(dt, 25000.0f);
    trans += (172.0f + 2.0f * load - trans) * lag(dt, 45000.0f);

    // The alternator holds ~14.2 V, sagging a little at idle with the fans and lights on.
    float volts_target = 13.8f + 0.5f * LV_CLAMP(0.0f, (rpm - IDLE_RPM) / 1500.0f, 1.0f);
    volts += (volts_target - volts) * lag(dt, 800.0f);

    // Oil pressure rises with rpm until the relief valve caps it; cold oil reads higher.
    float cold_bonus = 12.0f * LV_CLAMP(0.0f, (190.0f - coolant) / 70.0f, 1.0f);
    float oil_target = LV_MIN(62.0f, 22.0f + rpm * 0.010f) + cold_bonus;
    oil += (oil_target - oil) * lag(dt, 150.0f);

    readings[GAUGE_BOOST] = boost + rnd(-1, 1) * (boost > 2.0f ? 0.18f : 0.08f);
    readings[GAUGE_AFR] = afr + rnd(-0.06f, 0.06f);
    readings[GAUGE_COOLANT] = coolant;
    readings[GAUGE_RPM] = rpm + rnd(-12, 12);
    readings[GAUGE_TRANS] = trans;
    readings[GAUGE_SPEED] = speed;
    readings[GAUGE_VOLTAGE] = volts + rnd(-0.03f, 0.03f);
    readings[GAUGE_OIL_PRESSURE] = oil + rnd(-0.4f, 0.4f);
    return readings;
}
