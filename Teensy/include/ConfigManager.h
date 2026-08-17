#pragma once
#include <ArduinoJson.h>
#include <Arduino.h>

// Loads/exposes CarCANdy's JSON config from the Teensy's SD card. Schema
// matches DOC/config.example.json.
class ConfigManager {
public:
    struct CanBusConfig {
        uint8_t  id;
        bool     enabled;
        uint32_t baud;
        const char* mode; // "read_only" or "transmit"
    };

    bool begin();
    bool save(); // serializes doc_ back to /config.json on SD

    // Replaces the in-memory config with the given JSON bytes and saves it.
    // Used to apply a CONFIG_SET packet from the ESP32. Returns false (and
    // leaves the existing config untouched) if the JSON fails to parse.
    bool applyJson(const uint8_t* data, size_t len);

    // Serializes the current config as JSON into outBuf. Returns the number
    // of bytes written, or 0 if it doesn't fit in outBufLen.
    size_t toJson(char* outBuf, size_t outBufLen) const;

    const char* commMode() const; // "UART" or "SPI"
    const char* loggingPrefix() const; // base filename for CSV logs, e.g. "canlog"
    const char* filterMode() const; // "allow", "deny", or "none"
    const char* filterFile() const; // path to the BUS,ID filter CSV on SD

    // Whether to also mirror all non-CAN-frame Serial output to a
    // /debug_NN.log file on SD (see Logging). Off by default -- opt-in
    // since it's extra SD wear/writes most users won't need.
    bool debugEnabled() const;

    // Looks up a bus by its "id" field (1-3, matching the CAN controller --
    // see CANManager.h) rather than array position, so canBuses entries can
    // be in any order. Returns a disabled/read_only default if no entry
    // with that id exists.
    CanBusConfig canBus(uint8_t busNumber) const;
    bool isPluginEnabled(const char* name) const;
    JsonObjectConst pluginConfig(const char* name) const;

    // Ensures plugins.<name> exists, with "enabled" defaulting to
    // enabledDefault if not already present, and returns its "config"
    // subobject (existing or newly created) for the caller to merge
    // per-field defaults into. Safe to call every boot -- never clears or
    // overwrites existing saved values, only fills in what's missing. See
    // PluginBase::setDefaultConfig()/setDefault(), which is how
    // PluginManager::begin() uses this.
    JsonObject pluginDefaultsTarget(const char* name, bool enabledDefault);

private:
    void loadDefaults();

    JsonDocument doc_;
};
