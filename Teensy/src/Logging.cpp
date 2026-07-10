#include "Logging.h"

bool Logging::begin(const char* prefix) {
    sdReady_ = SD.begin(BUILTIN_SDCARD);
    if (!sdReady_) {
        Serial.println("[Logging] SD card not available, logging to Serial only");
        return false;
    }

    const int index = findNextIndex(prefix);
    pruneOldLogs(prefix, index);

    char path[64];
    snprintf(path, sizeof(path), "/%s_%02d.csv", prefix, index);

    logFile_ = SD.open(path, FILE_WRITE);
    if (!logFile_) {
        Serial.printf("[Logging] failed to open %s\n", path);
        sdReady_ = false;
        return false;
    }
    logFile_.println("timestampMs,bus,id,extended,rtr,dlc,data");
    logFile_.flush();
    Serial.printf("[Logging] logging to %s\n", path);

    lastFlushMs_ = millis();
    return true;
}

int Logging::findNextIndex(const String& prefix) const {
    File root = SD.open("/");
    if (!root) {
        return 0;
    }

    const String wantPrefix = prefix + "_";
    int maxIndex = -1;
    for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
        if (!entry.isDirectory()) {
            String name = entry.name();
            if (name.startsWith(wantPrefix) && name.endsWith(".csv")) {
                const int idx = name.substring(wantPrefix.length(), name.length() - 4).toInt();
                if (idx > maxIndex) {
                    maxIndex = idx;
                }
            }
        }
        entry.close();
    }
    root.close();
    return maxIndex + 1;
}

void Logging::pruneOldLogs(const String& prefix, int newIndex) const {
    // Keep only the kMaxLogFiles most recent indices, i.e.
    // [newIndex - kMaxLogFiles + 1 .. newIndex]; delete anything older.
    const int keepFromIndex = newIndex - kMaxLogFiles + 1;
    if (keepFromIndex <= 0) {
        return; // fewer than kMaxLogFiles have ever been created
    }

    File root = SD.open("/");
    if (!root) {
        return;
    }

    const String wantPrefix = prefix + "_";
    for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
        if (!entry.isDirectory()) {
            String name = entry.name();
            if (name.startsWith(wantPrefix) && name.endsWith(".csv")) {
                const int idx = name.substring(wantPrefix.length(), name.length() - 4).toInt();
                if (idx < keepFromIndex) {
                    const String path = "/" + name;
                    entry.close();
                    SD.remove(path.c_str());
                    continue;
                }
            }
        }
        entry.close();
    }
    root.close();
}

void Logging::loop() {
    if (!sdReady_) {
        return;
    }
    if (millis() - lastFlushMs_ >= kFlushIntervalMs) {
        logFile_.flush();
        lastFlushMs_ = millis();
    }
}

void Logging::info(const char* msg) {
    Serial.print("[INFO] ");
    Serial.println(msg);
}

void Logging::logCanFrame(const CarCANdy::CanFrame& frame) {
    Serial.printf("[CAN] bus=%u id=0x%03lX dlc=%u\n", frame.bus, frame.id, frame.dlc);

    if (!sdReady_) {
        return;
    }
    logFile_.printf("%lu,%u,0x%03lX,%u,%u,%u,", frame.timestampMs, frame.bus, frame.id, frame.extended, frame.rtr, frame.dlc);
    for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
        logFile_.printf("%02X", frame.data[i]);
    }
    logFile_.println();
    // Not flushed here -- see loop() for the periodic flush strategy.
}
