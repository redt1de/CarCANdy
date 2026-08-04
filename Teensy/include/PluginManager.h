#pragma once
#include "PluginBase.h"
#include "ConfigManager.h"

class CANManager;

// Fixed-size registry (no heap churn on an embedded target). main.cpp calls
// registerPlugin() once per known plugin at startup.
class PluginManager {
public:
    PluginManager(ConfigManager& config, CANManager& canManager);

    bool registerPlugin(PluginBase* plugin);
    void begin();
    void loop();
    void dispatchCanFrame(const CarCANdy::CanFrame& frame);

private:
    static constexpr uint8_t kMaxPlugins = 8;

    ConfigManager& config_;
    CANManager&    canManager_;
    PluginBase*    plugins_[kMaxPlugins] = {};
    uint8_t        pluginCount_ = 0;
};
