#pragma once
#include "CanTypes.h"
#include <ArduinoJson.h>

class ConfigManager;
class CANManager;

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
    virtual void init(ConfigManager& config, CANManager& canManager) {}
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
