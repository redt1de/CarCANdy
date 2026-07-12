
0x230 (HS1, 20ms):
b0: current gear (224 - R, 16 - 1, 32 - 2, ..., bit0 for clutch disengaged)
b1: gearbox mode (2 - R, 4 - N, 6 - D, 8 - S)
b4: transmission fluid temperature ((b4-60)°C)

0x204 (HS1 & HS3, 10ms):
b0<<8+b1: accelerator pedal (0-100): ((b0&3)<<8 + b1) / 10
b3<<8+b4: matches RPM, smoother up/down edges: (b3<<8 + b4)*2


0x171 (HS1 & HS3, 30ms):
b0: current gear (20 - P/N, 36, 52, 68, 84, 100 - for gears), bit0 for manual
mode, bit1 if attempt to change gear fails b1: gearbox mode (0 - P, 32 - R, 64 -
N, 96 - D, 128 - S)


---- track mode ----
0x20A (HS3):
b0: drive modes (1 - normal, 17 - sport/track, 33 - snow)
b1: represents current gear, bit7 - drops sometimes when gears change, bit2
drops sometimes

0x3C8 (HS1, 1000ms):
b0, b4, b5 - RGB colors of My Color
b2 - drive mode (0 - normal, 1 - sport, 2 - snow, 3 - track)

CanID CanIDs[NumCanIDs] = {
    {"Currrent Gear", 0x230, &CalcCurrentGear, PrintCurrentGear},
    {"Engine RPM", 0x204, &CalcEngineRPM, PrintEngineRPM},
    {"Gearbox Mode", 0x171, &CalcGearboxMode, PrintGearboxMode},
    {"Drive Mode", 0x3C8, &CalcCurrentDriveMode, PrintCurrentDriveMode}};