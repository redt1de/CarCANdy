#pragma once
#include "CanTypes.h"
#include <stddef.h>

class ConfigManager;
class FilterManager;

// When enabled via config.json's "simulate" flag (WebUI: Config page),
// listens on the Teensy's USB Serial for fabricated CAN frames and feeds
// them into the exact same path real bus traffic takes -- FilterManager's
// allow/deny check, then the frame callback (logging, ESP32 streaming,
// plugin dispatch; see main.cpp's onCanFrame()). Lets the full pipeline be
// exercised by replaying a recorded CSV log (see the "candb simulate" CLI
// subcommand) with no car connected. Off by default; re-checks
// config.simulateEnabled() every loop() (like PluginManager does for each
// plugin's enabled flag), so toggling it from the WebUI takes effect
// immediately -- no reboot needed.
//
// Wire format: one ASCII line per frame on Serial,
//   SIM,<bus>,<id>,<extended>,<rtr>,<dlc>,<data>
// bus/extended/rtr/dlc are decimal, id is hex (0x-prefixed or not), data is
// a concatenated hex string with no separators -- the same encoding
// Logging::logCanFrame()'s CSV row uses for those same columns, minus the
// timestamp (injected frames are stamped with this Teensy's own millis()
// on arrival, just like real RX, rather than trusting a replayed one).
// Lines not starting with "SIM," are ignored, so this can't be confused
// with the "[...]"-prefixed debug text Logging also tees to Serial.
class Simulator {
public:
    using FrameCallback = void (*)(const CarCANdy::CanFrame& frame);

    void begin(ConfigManager& config, FilterManager& filterManager, FrameCallback callback);
    void loop();

private:
    ConfigManager* config_ = nullptr;
    FilterManager* filterManager_ = nullptr;
    FrameCallback callback_ = nullptr;
    bool lastEnabled_ = false;
    char lineBuf_[160];
    size_t lineLen_ = 0;
};
