#pragma once
#include <Arduino.h>
#include "ConfigManager.h"

// Software-based BUS,ID CAN frame filtering (see DOC/protocol.md and the
// filtering design discussion for why this is software-side rather than
// FlexCAN mailbox/FIFO hardware filters -- mainly that hardware filter
// slots are far too few for an arbitrary user-defined list).
//
// Applied once, as early as possible in CANManager, before a frame reaches
// logging, the ESP32 stream, or any plugin -- there is exactly one filter
// point, not a per-consumer filter.
//
// Config-driven mode/file (ConfigManager::filterMode()/filterFile()) is
// the persisted default. A runtime-only override (setOverride()/
// clearOverride()) lets the WebUI's Sniffer page temporarily show
// everything for testing without touching the saved config -- see
// CommManager's CMD handling for how the ESP32 drives this.
class FilterManager {
public:
    enum class Mode : uint8_t { NONE, ALLOW, DENY };

    explicit FilterManager(ConfigManager& config);

    // Loads mode/file from config and reads the filter CSV from SD.
    void begin();

    // Re-reads the CSV file from SD (e.g. after the user hand-edited it,
    // or via the RELOAD_FILTERS CMD) without needing a reboot.
    void reload();

    // The core decision point, called from CANManager for every received
    // frame before it's forwarded anywhere.
    bool shouldPass(uint8_t bus, uint32_t id) const;

    // Runtime-only override of the effective mode -- does not touch
    // config.json. Pass Mode::NONE via setOverride() to temporarily show
    // everything; clearOverride() reverts to the configured mode.
    void setOverride(Mode mode);
    void clearOverride();

    Mode configuredMode() const { return configuredMode_; }
    Mode effectiveMode() const { return hasOverride_ ? overrideMode_ : configuredMode_; }

    // Adds/removes a BUS,ID (or BUS,ID_START-ID_END) entry, persisting the
    // change to the CSV file on SD. Returns false if the entry table is
    // full (add) or the spec couldn't be parsed.
    bool addFilterFromString(const char* spec);
    bool removeFilterFromString(const char* spec);

    static bool parseModeName(const char* name, Mode& out);
    static const char* modeName(Mode mode);

private:
    struct Entry {
        uint8_t  bus;
        uint32_t idStart;
        uint32_t idEnd;
    };

    static constexpr size_t kMaxEntries = 128;

    bool loadFromFile();
    bool writeToFile() const;
    bool matchesAnyEntry(uint8_t bus, uint32_t id) const;
    bool parseEntrySpec(const char* spec, Entry& out) const;
    bool addFilter(uint8_t bus, uint32_t idStart, uint32_t idEnd);
    bool removeFilter(uint8_t bus, uint32_t idStart, uint32_t idEnd);

    ConfigManager& config_;
    Mode configuredMode_ = Mode::NONE;
    bool hasOverride_ = false;
    Mode overrideMode_ = Mode::NONE;
    char filePath_[64] = "/filters.csv";

    Entry  entries_[kMaxEntries];
    size_t entryCount_ = 0;
};
