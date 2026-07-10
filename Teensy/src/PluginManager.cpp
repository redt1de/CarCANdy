#include "PluginManager.h"
#include <Arduino.h>

PluginManager::PluginManager(ConfigManager& config) : config_(config) {}

bool PluginManager::registerPlugin(PluginBase* plugin) {
    if (pluginCount_ >= kMaxPlugins) {
        Serial.println("[PluginManager] plugin registry full");
        return false;
    }
    plugins_[pluginCount_++] = plugin;
    return true;
}

void PluginManager::begin() {
    for (uint8_t i = 0; i < pluginCount_; ++i) {
        plugins_[i]->init(config_);
    }
}

void PluginManager::loop() {
    for (uint8_t i = 0; i < pluginCount_; ++i) {
        if (config_.isPluginEnabled(plugins_[i]->getName())) {
            plugins_[i]->loop();
        }
    }
}

void PluginManager::dispatchCanFrame(const CarCANdy::CanFrame& frame) {
    for (uint8_t i = 0; i < pluginCount_; ++i) {
        if (config_.isPluginEnabled(plugins_[i]->getName())) {
            plugins_[i]->onCanFrame(frame);
        }
    }
}
