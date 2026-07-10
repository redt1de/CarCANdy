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
    CanBusConfig canBus(uint8_t index) const;
    bool isPluginEnabled(const char* name) const;
    JsonObjectConst pluginConfig(const char* name) const;

private:
    void loadDefaults();

    JsonDocument doc_;
};
