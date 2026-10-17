// Calculate car data values from OBD2 data. These calculations specifically
// work for the 2016 Ford Mustang Ecoboost. They might not work on other cars,
// feel free to experiment

#ifndef _OBD2_CALCULATIONS
#define _OBD2_CALCULATIONS
#include <Arduino.h>

// debug helper to dump raw pData
inline void DumpPData(const char *label, const uint8_t *pData,
                      uint8_t len = 8) {

  Serial.print(label);
  Serial.print(": ");

  for (uint8_t i = 0; i < len; i++) {
    Serial.printf("%02X ", pData[i]);
  }
  Serial.println();
}

// --------------------------------------------------------
// ******** Engine RPM ************************************
// --------------------------------------------------------

static int32_t g_EngineRPM = 0;

int32_t CalcEngineRPM(const uint8_t *pData) {

  uint8_t A = pData[3];
  uint8_t B = pData[4];
  g_EngineRPM = (A * 256 + B) * 2;

  DumpPData("EngineRPM pData", pData);

  return g_EngineRPM;
}

void PrintEngineRPM() { Serial.printf("Engine RPM = %d\n", g_EngineRPM); }

// --------------------------------------------------------
// ******** Currently Engaged Gear ************************
// --------------------------------------------------------

static int32_t g_CurrentGear = 0; // 0 = Neutral, -1 = Reverse

int32_t CalcCurrentGear(const uint8_t *pData) {
  if (pData[1] == 2) {
    g_CurrentGear = -1; // Reverse
  } else if (pData[1] == 4) {
    g_CurrentGear = 0; // Neutral
  } else {
    g_CurrentGear = pData[0] / 16;
  }

  DumpPData("CurrentGear pData", pData);

  return g_CurrentGear;
}

void PrintCurrentGear() {
  char gearStr[32] = "Neutral";

  if (g_CurrentGear == -1) {
    sprintf(gearStr, "Reverse");
  } else if (g_CurrentGear > 0) {
    sprintf(gearStr, "%d", g_CurrentGear);
  }
#ifdef DEBUG
  Serial.printf("Current Engaged Gear = %s\n", gearStr);
#endif
}

// --------------------------------------------------------
// ******** Gearbox Mode ********************************
// --------------------------------------------------------

// Gearbox  mode
// enum GearboxMode {
//   P = 0x00, // Park
//   R = 0x20, // Reverse
//   N = 0x40, // Neutral
//   D = 0x60, // Drive
//   S = 0x80, // Sport
// };

enum GearboxMode {
  P = 0, // Park (if present as 0)
  R = 2,
  N = 4,
  D = 6,
  S = 8,
};

static int32_t g_GearboxMode = GearboxMode::P;

int32_t CalcGearboxMode(const uint8_t *pData) {
  g_GearboxMode = pData[1];

  DumpPData("GearboxMode pData", pData);
  return g_GearboxMode;
}

void PrintGearboxMode() {
  Serial.printf("Gearbox Mode: %#04x %s\n", g_GearboxMode,
                (g_GearboxMode == GearboxMode::P)   ? "P"
                : (g_GearboxMode == GearboxMode::R) ? "R"
                : (g_GearboxMode == GearboxMode::N) ? "N"
                : (g_GearboxMode == GearboxMode::D) ? "D"
                : (g_GearboxMode == GearboxMode::S)
                    ? "S"
                    : "ERROR: Unknown Drive Mode");
}

// --------------------------------------------------------
// ******** Drive Mode ********************************
// --------------------------------------------------------

// 0x3C8 (HS1, 1000ms):
// b0, b4, b5 - RGB colors of My Color
// b2 - drive mode (0 - normal, 1 - sport, 2 - snow, 3 - track)

static int32_t g_CurrentDriveMode = 0;

int32_t CalcCurrentDriveMode(const uint8_t *pData) {
  // 0x3C8 byte2 appears to be flags + mode. Mode is commonly in the low 2 bits.
  uint8_t raw = pData[2];
  uint8_t mode = raw & 0x03;

  g_CurrentDriveMode = mode;

  // Optional debug (super helpful while you flip modes)
  DumpPData("CurrentDriveMode pData", pData);

  return g_CurrentDriveMode;
}

void PrintCurrentDriveMode() {
  switch (g_CurrentDriveMode) {
  case 0:
    Serial.println("Normal");
    break;
  case 1:
    Serial.println("Sport+");
    break;
  case 2:
    Serial.println("Track");
    break;
  case 3:
    Serial.println("Snow/Wet");
    break;
  default:
    Serial.println("Unknown");
    break;
  }
}

#endif // _OBD2_CALCULATIONS
