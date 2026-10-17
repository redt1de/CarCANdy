#include "CANManager.h"
#include "CommManager.h"
#include "ConfigManager.h"
#include "FilterManager.h"
#include "Logging.h"
#include "PluginManager.h"
#include "plugins/ShiftLightPlugin.h"
#include <Arduino.h>

#define RESET_PIN 33

ConfigManager config;
FilterManager filterManager(config);
CANManager canManager(config, filterManager);
CommManager commManager(config, canManager, filterManager);
PluginManager pluginManager(config, canManager, logging);
ShiftLightPlugin shiftLightPlugin;

void onCanFrame(const CarCANdy::CanFrame &frame) {
  logging.logCanFrame(frame);
  commManager.streamCanFrame(frame);
  pluginManager.dispatchCanFrame(frame);
}

// Forwards every line Logging prints (minus CAN frames, which bypass this
// by design) to the ESP32 as a LOG_LINE packet, for the WebUI's Logging
// page. commManager.sendLogLine() is null-guarded, so this is safe to wire
// up before commManager.begin() has run.
void onLogLine(const char *line, size_t len) {
  commManager.sendLogLine(line, len);
}

// // <<<<<<<<<<<<<< REMOVE ME -- bench test only, no CAN bus needed.
// // Feeds a synthetic 0x204 RPM frame through the same onCanFrame() pipeline a
// // real frame takes (logging, ESP32 stream, plugin dispatch), sweeping
// // 500-9000 RPM up and down so ShiftLightPlugin's tach/flash modes can be
// // exercised on the bench.
void testShiftLightSweep() {
  constexpr int32_t kMinRpm = 500;
  constexpr int32_t kMaxRpm = 9000;
  constexpr int32_t kStepRpm = 50;
  constexpr unsigned long kStepIntervalMs =
      20; // roughly matches 0x204's real cadence

  static int32_t rpm = kMinRpm;
  static int8_t direction = 1;
  static unsigned long lastStepMs = 0;

  const unsigned long now = millis();
  if (now - lastStepMs < kStepIntervalMs) {
    return;
  }
  lastStepMs = now;

  rpm += direction * kStepRpm;
  if (rpm >= kMaxRpm) {
    rpm = kMaxRpm;
    direction = -1;
  } else if (rpm <= kMinRpm) {
    rpm = kMinRpm;
    direction = 1;
  }

  // Inverse of ShiftLightPlugin::onCanFrame()'s (A*256+B)*2 decode.
  const uint16_t raw = static_cast<uint16_t>(rpm / 2);

  CarCANdy::CanFrame frame{};
  frame.bus = 1;
  frame.id = 0x204;
  frame.dlc = 8;
  frame.timestampMs = now;
  frame.data[3] = static_cast<uint8_t>(raw >> 8);
  frame.data[4] = static_cast<uint8_t>(raw & 0xFF);

  onCanFrame(frame);
}

// == REMMOVE ME ==
// relays: 12,26,27
// inputs: 2,3,4
// adcs: A1,A0

#define RELAY1 12
#define RELAY2 26
#define RELAY3 27
#define IN1 2
#define IN2 3
#define IN3 4
// == END REMMOVE ME ==

void setup() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);

  config.begin();
  logging.begin(config.loggingPrefix(), config.debugEnabled());
  logging.setLineCallback(onLogLine);
  filterManager.begin();
  canManager.setFrameCallback(onCanFrame);
  canManager.begin();
  commManager.begin();

  pluginManager.registerPlugin(&shiftLightPlugin);
  pluginManager.begin();

  logging.info("CarCANdy Teensy firmware started");
}

void handleReboot() {
  if (digitalRead(RESET_PIN) == LOW) {
    SCB_AIRCR = 0x05FA0004; // Cortex-M7 System Reset
    while (1) {
    }
  }
}

// Fast blink = CSV logging isn't currently up (no SD card, SD.open()
// failed, or it died and hasn't recovered yet -- see Logging::loop()'s
// self-healing retry); solid on = fine. Checks logging.sdReady() live
// every call rather than a one-time boot snapshot, so this reacts the
// moment logging actually stops or recovers, not just at boot -- that
// snapshot previously made this LED blind to logging dying mid-drive.
void updateStatusLed() {
  if (logging.sdReady()) {
    digitalWrite(LED_BUILTIN, HIGH);
    return;
  }
  constexpr unsigned long kBlinkIntervalMs = 200;
  static unsigned long lastToggleMs = 0;
  static bool ledOn = true;
  const unsigned long now = millis();
  if (now - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs = now;
    ledOn = !ledOn;
    digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
  }
}

void loop() {
  handleReboot();

  canManager.loop();
  commManager.poll();
  pluginManager.loop();
  logging.loop();
  updateStatusLed();
  // // testShiftLightSweep(); // <<<<<<<<<<<<<< REMOVE ME
}
