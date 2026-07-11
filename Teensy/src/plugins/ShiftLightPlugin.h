#pragma once
#include "ConfigManager.h"
#include "PluginBase.h"
#include <ArduinoJson.h>

// Anonymous enum with an explicit uint8_t underlying type -- NOT `enum
// uint8_t { ... }`, which would declare a *type* named uint8_t and collide
// with the real uint8_t typedef from stdint.h.
enum : uint8_t {
  MODE_FLASH, // Defaults to 0
  MODE_TACH,  // Defaults to 1
};

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
    setDefault(cfg, "shiftLightMode", static_cast<uint8_t>(MODE_FLASH));
  }

  void init(ConfigManager &config) override {
    // By the time init() runs, PluginManager::begin() has already merged
    // this plugin's defaults in via setDefaultConfig(), so no `| fallback`
    // is needed here -- these keys are guaranteed present.
    JsonObjectConst cfg = config.pluginConfig(getName());
    shiftThreshold_ = cfg["shiftThreshold"];
    redlineThreshold_ = cfg["redlineThreshold"];
    trackModeOnly_ = cfg["trackModeOnly"];
    shiftLightMode_ = cfg["shiftLightMode"];
  }

  void loop() override {
    // TODO(future): drive a shift light GPIO output based on latest RPM.
    if (shiftLightMode_ == MODE_FLASH) {
      // just flash
    } else {
      // do tach stuff
    }
  }

  void onCanFrame(const CarCANdy::CanFrame &frame) override {
    // TODO(future): check frame.id against the RPM PID for this vehicle,
    // compare against shiftThreshold_/redlineThreshold_, and drive the
    // shift light GPIO according to shiftLightMode_.
    (void)frame;
  }

private:
  uint16_t shiftThreshold_ = 6500;
  uint16_t redlineThreshold_ = 6500;
  bool trackModeOnly_ = true;
  uint8_t shiftLightMode_ = MODE_FLASH;
};
