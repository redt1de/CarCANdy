#pragma once
#include "CanTypes.h"
#include <ArduinoJson.h>

class ConfigManager;
class CANManager;
class Logging;

// Base interface for all plugins (shift light, line lock, etc). Plugins are
// always compiled in and registered unconditionally; enable/disable is a
// runtime config check done by PluginManager, so the web UI's Plugins tab
// can toggle them without reflashing.
class PluginBase {
public:
    virtual ~PluginBase() = default;

    virtual const char* getName() const = 0;

    // canManager is provided so a plugin can transmit (canManager.sendFrame())
    // in response to config/CAN events. CANManager::sendFrame() itself
    // enforces each bus's configured "transmit" mode, so this doesn't hand a
    // plugin any more TX authority than the user's own bus config allows.
    //
    // logging is provided so a plugin can safely use the SD card for its
    // own files: wrap any SD.open() in a LoggingPause first, e.g.
    //   { LoggingPause pause(logging); File f = SD.open("/my_plugin.csv", FILE_WRITE); ... }
    // See Logging.h for why this is required -- CarCANdy's own CSV/debug
    // logs are held open for the entire runtime, and Teensy's SD library
    // has been observed to corrupt data across files if a second file is
    // opened/written while those are open.
    virtual void init(ConfigManager& config, CANManager& canManager, Logging& logging) {}
    virtual void loop() {}
    virtual void onCanFrame(const CarCANdy::CanFrame& frame) {}

    // Whether this plugin should default to enabled the first time it's
    // seen (no existing config entry yet). Most plugins should stay
    // opt-in (the default, false).
    virtual bool defaultEnabled() const { return false; }

    // Called once at startup by PluginManager::begin(), before init(), so
    // this plugin's own default config values live with the plugin
    // instead of being hardcoded centrally in ConfigManager -- that's the
    // whole point: adding or changing a plugin's config never requires
    // touching ConfigManager. Override this and call setDefault() below
    // for each field.
    virtual void setDefaultConfig(JsonObject cfg) const {}

protected:
    // Fills in `key` only if it's not already present -- so re-running
    // this every boot never clobbers a value the user has since changed
    // and saved.
    template <typename T>
    void setDefault(JsonObject cfg, const char* key, T value) const {
        if (cfg[key].isNull()) {
            cfg[key] = value;
        }
    }
};
