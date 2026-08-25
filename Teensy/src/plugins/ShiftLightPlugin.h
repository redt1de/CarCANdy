#pragma once
#include "AuxPorts.h"
#include "CANManager.h"
#include "ConfigManager.h"
#include "PluginBase.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <string.h>

// 24-LED neopixel strip, defaults to AUX2's TX pin -- see "ledPin" in
// setDefaultConfig() below, configurable from the WebUI/config.json without
// a reflash.

/*

RPM: HS1 -> 0x204 ->

0x204 (HS1 & HS3, 10ms):
b0<<8+b1: accelerator pedal (0-100): ((b0&3)<<8 + b1) / 10
b3<<8+b4: matches RPM, smoother up/down edges: (b3<<8 + b4)*2


*/

class ShiftLightPlugin : public PluginBase {
public:
  const char *getName() const override { return "shiftLight"; }

  // Single source of truth for this plugin's default config -- called
  // once at startup by PluginManager::begin() to seed/merge these into
  // the config doc (only for fields not already present there), so
  // ConfigManager never needs to know this plugin exists.
  void setDefaultConfig(JsonObject cfg) const override {
    setDefault(cfg, "shiftThreshold", 6500);
    setDefault(cfg, "redlineThreshold", 7500);
    setDefault(cfg, "trackModeOnly", true);
    // tachEnabled: false = strip stays off below shiftThreshold, alarm only.
    //              true  = progressive green bargraph below shiftThreshold.
    setDefault(cfg, "tachEnabled", false);
    // alarmMode: "flash" or "solid" -- how the strip behaves once
    // shiftThreshold/redlineThreshold is reached, in both tach modes.
    setDefault(cfg, "alarmMode", "flash");
    setDefault(cfg, "ledPin", kDefaultLedPin);
  }

  void init(ConfigManager &config, CANManager &canManager, Logging &logging) override {
    // By the time init() runs, PluginManager::begin() has already merged
    // this plugin's defaults in via setDefaultConfig(), so no `| fallback`
    // is needed here -- these keys are guaranteed present.
    JsonObjectConst cfg = config.pluginConfig(getName());
    shiftThreshold_ = cfg["shiftThreshold"];
    redlineThreshold_ = cfg["redlineThreshold"];
    trackModeOnly_ = cfg["trackModeOnly"];
    tachEnabled_ = cfg["tachEnabled"];
    alarmSolid_ = strcmp(cfg["alarmMode"] | "flash", "solid") == 0;
    ledPin_ = cfg["ledPin"];

    // strip_ is constructed at startup with kDefaultLedPin (Adafruit_NeoPixel
    // needs a pin at construction time, before config is loaded); setPin()
    // re-targets it to the configured pin before begin() latches it in.
    strip_.setPin(ledPin_);
    strip_.begin();
    strip_.show(); // all off
  }

  void loop() override {
    const uint16_t rpm =
        g_EngineRPM < 0 ? 0 : static_cast<uint16_t>(g_EngineRPM);

    if (rpm >= redlineThreshold_) {
      alarm(strip_.Color(255, 0, 0)); // red
    } else if (rpm >= shiftThreshold_) {
      alarm(strip_.Color(255, 255, 0)); // yellow
    } else if (tachEnabled_) {
      tachBargraph(rpm);
    } else {
      setAll(0); // off
    }
    // TODO(future): trackModeOnly_ isn't wired to anything yet -- there's no
    // CAN signal for drive mode we trust enough yet (see DOC/CANDB's 0x20A/
    // 0x3C8 drive_mode entries, both community-single-source/unverified).
    // Once one is confirmed against this car, gate the behavior above on it
    // when trackModeOnly_ is true.
  }

  void onCanFrame(const CarCANdy::CanFrame &frame) override {
    if (frame.bus == 1 && frame.id == 0x204) {
      uint8_t A = frame.data[3];
      uint8_t B = frame.data[4];
      g_EngineRPM = (A * 256 + B) * 2;
    }
  }

private:
  static constexpr uint8_t kLedCount = 24;
  static constexpr uint8_t kDefaultLedPin = AuxPorts::AUX2_TX_PIN;
  static constexpr unsigned long kFlashIntervalMs = 100;

  // Green bargraph over [0, shiftThreshold_) -- LED i lights once the
  // current RPM reaches that LED's slice of the range, so the strip fills
  // left-to-right and reaches full right at shiftThreshold_ (where alarm()
  // takes over instead).
  void tachBargraph(uint16_t rpm) {
    const uint32_t maxRpm = shiftThreshold_ > 0 ? shiftThreshold_ : 1;
    uint32_t litCount = (static_cast<uint32_t>(rpm) * kLedCount) / maxRpm;
    if (litCount > kLedCount) {
      litCount = kLedCount;
    }
    for (uint8_t i = 0; i < kLedCount; ++i) {
      strip_.setPixelColor(i, i < litCount ? strip_.Color(0, 255, 0) : 0);
    }
    strip_.show();
  }

  // At/past shiftThreshold_ or redlineThreshold_, in both tachEnabled_
  // states: all LEDs either flash or stay solid in the given color,
  // depending on alarmSolid_.
  void alarm(uint32_t color) {
    if (alarmSolid_) {
      setAll(color);
    } else {
      flashAll(color);
    }
  }

  void setAll(uint32_t color) {
    for (uint8_t i = 0; i < kLedCount; ++i) {
      strip_.setPixelColor(i, color);
    }
    strip_.show();
  }

  // Toggles the whole strip between colorWhenOn and off every
  // kFlashIntervalMs.
  void flashAll(uint32_t colorWhenOn) {
    const unsigned long now = millis();
    if (now - lastFlashToggleMs_ >= kFlashIntervalMs) {
      lastFlashToggleMs_ = now;
      flashOn_ = !flashOn_;
    }
    setAll(flashOn_ ? colorWhenOn : 0);
  }

  uint16_t shiftThreshold_ = 6500;
  uint16_t redlineThreshold_ = 7500;
  bool trackModeOnly_ = true;
  bool tachEnabled_ = false;
  bool alarmSolid_ = false; // false = flash, true = solid
  uint8_t ledPin_ = kDefaultLedPin;
  int32_t g_EngineRPM = 0;

  bool flashOn_ = false;
  unsigned long lastFlashToggleMs_ = 0;

  Adafruit_NeoPixel strip_{kLedCount, kDefaultLedPin, NEO_GRB + NEO_KHZ800};
};
