#include "FilterManager.h"
#include "Logging.h"
#include <SD.h>
#include <string.h>
#include <stdlib.h>

FilterManager::FilterManager(ConfigManager& config) : config_(config) {}

void FilterManager::begin() {
    Mode mode;
    if (!parseModeName(config_.filterMode(), mode)) {
        logging.printf("[FilterManager] unknown filter mode '%s', defaulting to none\n", config_.filterMode());
        mode = Mode::NONE;
    }
    configuredMode_ = mode;

    strncpy(filePath_, config_.filterFile(), sizeof(filePath_) - 1);
    filePath_[sizeof(filePath_) - 1] = '\0';

    loadFromFile();

    if (configuredMode_ == Mode::ALLOW && entryCount_ == 0) {
        logging.println("[FilterManager] mode=allow but the filter list is empty -- ALL frames will be dropped until entries are added");
    }
    logging.printf("[FilterManager] mode=%s file=%s entries=%u\n", modeName(configuredMode_), filePath_,
                  static_cast<unsigned>(entryCount_));
}

void FilterManager::reload() {
    loadFromFile();
    logging.printf("[FilterManager] reloaded %u entries from %s\n", static_cast<unsigned>(entryCount_), filePath_);
}

bool FilterManager::shouldPass(uint8_t bus, uint32_t id) const {
    switch (effectiveMode()) {
    case Mode::NONE:
        return true;
    case Mode::ALLOW:
        return matchesAnyEntry(bus, id);
    case Mode::DENY:
        return !matchesAnyEntry(bus, id);
    }
    return true;
}

void FilterManager::setOverride(Mode mode) {
    hasOverride_ = true;
    overrideMode_ = mode;
    logging.printf("[FilterManager] override set to %s\n", modeName(mode));
}

void FilterManager::clearOverride() {
    hasOverride_ = false;
    logging.println("[FilterManager] override cleared, using configured mode");
}

bool FilterManager::addFilterFromString(const char* spec) {
    Entry entry;
    if (!parseEntrySpec(spec, entry)) {
        logging.printf("[FilterManager] could not parse filter spec '%s'\n", spec);
        return false;
    }
    return addFilter(entry.bus, entry.idStart, entry.idEnd);
}

bool FilterManager::removeFilterFromString(const char* spec) {
    Entry entry;
    if (!parseEntrySpec(spec, entry)) {
        logging.printf("[FilterManager] could not parse filter spec '%s'\n", spec);
        return false;
    }
    return removeFilter(entry.bus, entry.idStart, entry.idEnd);
}

bool FilterManager::parseModeName(const char* name, Mode& out) {
    if (strcmp(name, "allow") == 0) {
        out = Mode::ALLOW;
        return true;
    }
    if (strcmp(name, "deny") == 0) {
        out = Mode::DENY;
        return true;
    }
    if (strcmp(name, "none") == 0) {
        out = Mode::NONE;
        return true;
    }
    return false;
}

const char* FilterManager::modeName(Mode mode) {
    switch (mode) {
    case Mode::ALLOW:
        return "allow";
    case Mode::DENY:
        return "deny";
    case Mode::NONE:
    default:
        return "none";
    }
}

bool FilterManager::matchesAnyEntry(uint8_t bus, uint32_t id) const {
    for (size_t i = 0; i < entryCount_; ++i) {
        if (entries_[i].bus == bus && id >= entries_[i].idStart && id <= entries_[i].idEnd) {
            return true;
        }
    }
    return false;
}

// Parses "<bus>,<id>" or "<bus>,<idStart>-<idEnd>". IDs may be decimal or
// 0x-prefixed hex. Blank lines and lines starting with '#' are rejected
// (treated as "not an entry" by the caller, e.g. skipped during CSV load).
bool FilterManager::parseEntrySpec(const char* spec, Entry& out) const {
    char buf[48];
    strncpy(buf, spec, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\r' || buf[len - 1] == '\n' || buf[len - 1] == ' ' || buf[len - 1] == '\t')) {
        buf[--len] = '\0';
    }

    char* p = buf;
    while (*p == ' ' || *p == '\t') {
        ++p;
    }
    if (*p == '#' || *p == '\0') {
        return false;
    }

    char* comma = strchr(p, ',');
    if (!comma) {
        return false;
    }
    *comma = '\0';
    char* busStr = p;
    char* idStr = comma + 1;
    while (*idStr == ' ' || *idStr == '\t') {
        ++idStr;
    }

    char* busEnd = nullptr;
    long bus = strtol(busStr, &busEnd, 10);
    if (busEnd == busStr || bus < 0 || bus > 255) {
        return false;
    }

    unsigned long idStart;
    unsigned long idEnd;
    char* dash = strchr(idStr, '-');
    if (dash) {
        *dash = '\0';
        char* idEndStr = dash + 1;
        char* e1 = nullptr;
        char* e2 = nullptr;
        idStart = strtoul(idStr, &e1, 0);
        idEnd = strtoul(idEndStr, &e2, 0);
        if (e1 == idStr || e2 == idEndStr) {
            return false;
        }
    } else {
        char* e = nullptr;
        idStart = idEnd = strtoul(idStr, &e, 0);
        if (e == idStr) {
            return false;
        }
    }

    out.bus = static_cast<uint8_t>(bus);
    out.idStart = idStart;
    out.idEnd = idEnd;
    return true;
}

bool FilterManager::addFilter(uint8_t bus, uint32_t idStart, uint32_t idEnd) {
    if (entryCount_ >= kMaxEntries) {
        logging.println("[FilterManager] cannot add filter -- table full");
        return false;
    }
    entries_[entryCount_++] = Entry{bus, idStart, idEnd};
    return writeToFile();
}

bool FilterManager::removeFilter(uint8_t bus, uint32_t idStart, uint32_t idEnd) {
    bool removed = false;
    for (size_t i = 0; i < entryCount_;) {
        if (entries_[i].bus == bus && entries_[i].idStart == idStart && entries_[i].idEnd == idEnd) {
            for (size_t j = i; j + 1 < entryCount_; ++j) {
                entries_[j] = entries_[j + 1];
            }
            --entryCount_;
            removed = true;
        } else {
            ++i;
        }
    }
    if (!removed) {
        return false;
    }
    return writeToFile();
}

bool FilterManager::loadFromFile() {
    entryCount_ = 0;
    if (!SD.begin(BUILTIN_SDCARD)) {
        logging.println("[FilterManager] SD card not available");
        return false;
    }
    File f = SD.open(filePath_, FILE_READ);
    if (!f) {
        logging.printf("[FilterManager] %s not found -- filter list empty\n", filePath_);
        return false;
    }

    while (f.available()) {
        String line = f.readStringUntil('\n');
        Entry entry;
        if (!parseEntrySpec(line.c_str(), entry)) {
            continue; // blank line, comment, or malformed -- skip
        }
        if (entryCount_ >= kMaxEntries) {
            logging.println("[FilterManager] filter file has more entries than kMaxEntries -- truncating");
            break;
        }
        entries_[entryCount_++] = entry;
    }
    f.close();
    return true;
}

bool FilterManager::writeToFile() const {
    if (!SD.begin(BUILTIN_SDCARD)) {
        return false;
    }
    if (SD.exists(filePath_)) {
        SD.remove(filePath_);
    }
    File f = SD.open(filePath_, FILE_WRITE);
    if (!f) {
        return false;
    }
    for (size_t i = 0; i < entryCount_; ++i) {
        const Entry& e = entries_[i];
        if (e.idStart == e.idEnd) {
            f.printf("%u,0x%lX\n", e.bus, e.idStart);
        } else {
            f.printf("%u,0x%lX-0x%lX\n", e.bus, e.idStart, e.idEnd);
        }
    }
    f.close();
    return true;
}
