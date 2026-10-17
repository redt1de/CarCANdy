#include "PluginManager.h"
#include "Logging.h"
#include <Arduino.h>

PluginManager::PluginManager(ConfigManager& config, CANManager& canManager, Logging& logging)
    : config_(config), canManager_(canManager), logging_(logging) {}

bool PluginManager::registerPlugin(PluginBase* plugin) {
    if (pluginCount_ >= kMaxPlugins) {
        logging.print("[PluginManager] plugin registry full\n");
        return false;
    }
    plugins_[pluginCount_++] = plugin;
    return true;
}

void PluginManager::begin() {
    // Seed/merge each plugin's own defaults into the config *before* init()
    // reads it -- this is what lets adding or changing a plugin's config
    // stay entirely within the plugin's own file, no ConfigManager changes
    // needed. Safe to do unconditionally every boot: pluginDefaultsTarget()
    // and setDefault() only ever fill in what's missing.
    for (uint8_t i = 0; i < pluginCount_; ++i) {
        JsonObject cfg = config_.pluginDefaultsTarget(plugins_[i]->getName(), plugins_[i]->defaultEnabled());
        plugins_[i]->setDefaultConfig(cfg);
    }
    config_.save();

    for (uint8_t i = 0; i < pluginCount_; ++i) {
        plugins_[i]->init(config_, canManager_, logging_);
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
