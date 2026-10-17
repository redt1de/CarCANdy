#pragma once
#include "CANManager.h"
#include "ConfigManager.h"
#include "Logging.h"
#include "PluginBase.h"
#include <ArduinoJson.h>

// This file is intentionally not included/registered anywhere -- it's a
// reference to copy from, not a plugin that runs.
//
// Copy this file to start a new plugin:
//
//   1. Rename the file and the class (e.g. LineLockPlugin.h / LineLockPlugin).
//
//   2. Change getName() return value to something unique for ConfigJSON and
//      WebUI to use
//
//   3. Replace exampleThreshold_/exampleEnabled_ with whatever fields your
//      plugin actually needs, declaring their defaults in
//      setDefaultConfig() and reading them in init() the same way.
//      ConfigManager never needs to be touched -- PluginManager::begin()
//      calls setDefaultConfig() on every registered plugin at startup to
//      seed/merge its defaults in, which is also what makes the plugin
//      show up in the WebUI's Plugins tab (it's only visible there because
//      a config entry now exists for it).
//
//   4. include the new header and register an instance in Teensy/src/main.cpp:
//        #include "plugins/LineLockPlugin.h"
//        LineLockPlugin lineLockPlugin;
//        ...
//        pluginManager.registerPlugin(&lineLockPlugin);
//

class TemplatePlugin : public PluginBase {
public:
  const char *getName() const override { return "templatePlugin"; }

  // Optional: override if this plugin should default to enabled the
  // first time it's seen. Most plugins should stay opt-in (the base
  // class default, false) -- omit this override to get that.
  // bool defaultEnabled() const override { return false; }

  // Single source of truth for this plugin's default config values.
  // setDefault() only fills in a field if it's not already present, so
  // this is safe to leave running every boot -- a user's saved/changed
  // values are never overwritten.
  void setDefaultConfig(JsonObject cfg) const override {
    setDefault(cfg, "exampleThreshold", 100);
    setDefault(cfg, "exampleEnabled", true);
  }

  // Called once at startup, after setDefaultConfig() has already merged
  // this plugin's defaults in -- these keys are guaranteed present, so no
  // `| fallback` is needed here (and shouldn't be: keep the actual default
  // *values* only in setDefaultConfig() above, or the two can drift).
  //
  // canManager_/logging_ are stashed as pointers (not references -- this
  // plugin instance is default-constructed before init() runs, and
  // references can't be assigned after construction) so loop()/onCanFrame()
  // can use them later: canManager_->sendFrame() to transmit, or
  // LoggingPause(*logging_) to safely touch the SD card yourself.
  void init(ConfigManager &config, CANManager &canManager, Logging &logging) override {
    JsonObjectConst cfg = config.pluginConfig(getName());
    exampleThreshold_ = cfg["exampleThreshold"];
    exampleEnabled_ = cfg["exampleEnabled"];
    canManager_ = &canManager;
    logging_ = &logging;
  }

  // Called every main loop iteration, but only while this plugin is
  // enabled in config (plugins.<name>.enabled). Good for anything
  // time-based, like driving an output on an interval.
  void loop() override {
    // TODO: your periodic logic here.

    // Example: transmit a frame on bus 1. sendFrame() only actually sends
    // if bus 1 is configured with mode:"transmit" in config.json -- it's a
    // no-op (returns false) otherwise, so this can't override the user's
    // own bus config.
    // CarCANdy::CanFrame frame{};
    // frame.bus = 1;
    // frame.id = 0x123;
    // frame.dlc = 8;
    // frame.data[0] = 0xAA;
    // canManager_->sendFrame(frame.bus, frame);

    // Example: safely read/write your own file on the SD card. MUST wrap
    // any SD.open() in a LoggingPause first -- CarCANdy's own CSV/debug
    // logs are held open for the entire runtime, and Teensy's SD library
    // has been observed to corrupt data across files if a second file is
    // opened/written while those are open. See Logging.h for details.
    // {
    //   LoggingPause pause(*logging_);
    //   File f = SD.open("/my_plugin.csv", FILE_WRITE);
    //   if (f) {
    //     f.println("hello from a plugin");
    //     f.close();
    //   }
    // }
  }

  // Called for every CAN frame received on any enabled bus, but only
  // while this plugin is enabled. Good for reacting to specific signals.
  void onCanFrame(const CarCANdy::CanFrame &frame) override {
    // TODO: check frame.bus/frame.id for the signal(s) you care about,
    // decode frame.data, and act on it (e.g. compare against
    // exampleThreshold_, drive a GPIO).
    (void)frame;
  }

private:
  // Examples only -- a number and a bool read from config. Replace with
  // whatever your plugin actually needs.
  uint16_t exampleThreshold_ = 100;
  bool exampleEnabled_ = true;
  CANManager *canManager_ = nullptr;
  Logging *logging_ = nullptr;
};
