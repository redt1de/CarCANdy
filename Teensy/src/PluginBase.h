#pragma once
#include "CanTypes.h"

class ConfigManager;

// Base interface for all plugins (shift light, line lock, etc). Plugins are
// always compiled in and registered unconditionally; enable/disable is a
// runtime config check done by PluginManager, so the web UI's Plugins tab
// can toggle them without reflashing.
class PluginBase {
public:
    virtual ~PluginBase() = default;

    virtual const char* getName() const = 0;
    virtual void init(ConfigManager& config) {}
    virtual void loop() {}
    virtual void onCanFrame(const CarCANdy::CanFrame& frame) {}
};
