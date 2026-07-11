#pragma once
#include <Arduino.h>
#include <SD.h>
#include "CanTypes.h"

// Serial + SD-CSV logging. On begin(), scans the SD card for existing
// "<prefix>_NN.csv" files and creates the next-numbered one -- a fresh log
// file per boot rather than appending to/overwriting a single file -- then
// deletes older numbered files beyond kMaxLogFiles so the card doesn't fill
// up with logs from every past boot. The file is kept open for the life of
// the program; loop() must be called periodically to flush buffered writes
// to the card on an interval, so at most ~kFlushIntervalMs worth of frames
// are lost on a sudden power loss.
class Logging {
public:
    bool begin(const char* prefix);
    void loop();
    void info(const char* msg);
    void logCanFrame(const CarCANdy::CanFrame& frame);

private:
    static constexpr unsigned long kFlushIntervalMs = 1000;
    static constexpr int kMaxLogFiles = 10;

    int findNextIndex(const String& prefix) const;
    void pruneOldLogs(const String& prefix, int newIndex) const;

    bool sdReady_ = false;
    File logFile_;
    unsigned long lastFlushMs_ = 0;
};
