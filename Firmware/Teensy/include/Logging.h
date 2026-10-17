#pragma once
#include <Arduino.h>
#include <SD.h>
#include "CanTypes.h"

// Serial + SD-CSV logging, doubling as a tee-to-debug-log Print stream when
// enabled. Both files live under kLogDir ("/log") on SD, created if it
// doesn't exist yet. On begin(), scans that directory for existing
// "<prefix>_NN.csv" files and creates the next-numbered one -- a fresh log
// file per boot rather than appending to/overwriting a single file -- then
// deletes older numbered files beyond kMaxLogFiles so the card doesn't fill
// up with logs from every past boot. The file is kept open for the life of
// the program; loop() must be called periodically to flush buffered writes
// to the card on an interval, so at most ~kFlushIntervalMs worth of frames
// are lost on a sudden power loss.
//
// When ConfigManager's "debug" flag is enabled, begin() also creates
// "/log/debug_NN.log" using the SAME index NN as that boot's CAN CSV file,
// so the two can be lined up directly (e.g. log/mustang_03.csv <-> log/debug_03.log).
// print()/println()/printf() (inherited from Print, via write() below) tee
// every byte to both Serial and that debug file; other modules call these
// instead of Serial directly so their diagnostic output gets captured.
// logCanFrame() deliberately calls Serial directly (bypassing write()) so
// CAN frames are never duplicated into debug.log.
//
// IMPORTANT: Teensy's SD (SdFat) library appears to share a single sector
// cache across all open files -- opening/writing a *different* file while
// logFile_/debugFile_ are held open (which they are for the entire
// runtime) has been observed on real hardware to corrupt data across
// files (a config.json save got clobbered with debug-log text). Any code
// that does its own transient SD.open() -- ConfigManager, FilterManager,
// or a plugin -- MUST wrap it in a LoggingPause (below) so these two files
// are closed for the duration.
class Logging : public Print {
public:
    using LineCallback = void (*)(const char* line, size_t len);

    bool begin(const char* prefix, bool debugEnabled);
    void loop();
    void info(const char* msg);
    void logCanFrame(const CarCANdy::CanFrame& frame);

    // True once begin() has successfully created this boot's CSV log file
    // (i.e. SD card present and writable). Surfaced on the WebUI's Status
    // page via MsgType::STATUS -- see CommManager.
    bool sdReady() const { return sdReady_; }

    // Called once per complete line written via print()/println()/printf()
    // (not logCanFrame(), which bypasses this by design -- see class
    // comment). main.cpp wires this to CommManager::sendLogLine() so the
    // WebUI's Logging page can mirror this output live over WiFi.
    void setLineCallback(LineCallback cb) { lineCallback_ = cb; }

    // Closes logFile_/debugFile_ (flushing first) so another module can
    // safely open/write/close its own file on the SD card without a second
    // file being concurrently open. Safe to call even if nothing is open
    // (e.g. SD unavailable, or debug logging disabled) -- becomes a no-op.
    // Prefer LoggingPause over calling this directly, so resume() can't be
    // forgotten.
    void suspend();

    // Reopens (in append mode) whatever suspend() closed, retrying a few
    // times -- this has been observed on real hardware to occasionally
    // fail right after a close(). On persistent failure, cleanly marks
    // that subsystem down (sdReady()/debug) rather than leaving sdReady()
    // reporting true while logFile_ is silently a dead handle -- which
    // previously caused logging to go permanently (and silently) quiet for
    // the rest of the boot after the very first config save, since
    // PluginManager::begin() triggers one on every single boot. loop()
    // periodically retries recovering from that state -- see below.
    void resume();

    // Print interface -- powers print()/println()/printf() above.
    size_t write(uint8_t c) override;
    size_t write(const uint8_t* buffer, size_t size) override;

private:
    // Both the CAN CSV and debug.log live in this directory on SD (created
    // if missing) rather than the card's root.
    static constexpr const char* kLogDir = "/log";

    static constexpr unsigned long kFlushIntervalMs = 1000;
    static constexpr unsigned long kRecoveryIntervalMs = 5000;
    static constexpr int kMaxLogFiles = 10;
    static constexpr uint8_t kReopenAttempts = 3;
    static constexpr unsigned long kReopenRetryDelayMs = 10;

    int findNextIndex(const String& prefix, const char* extension) const;
    void pruneOldLogs(const String& prefix, const char* extension, int newIndex) const;
    // Retries SD.open(path, FILE_WRITE) a few times before giving up --
    // returns an invalid File if every attempt failed.
    File reopenWithRetry(const String& path) const;

    bool sdReady_ = false;
    bool csvEverOpened_ = false; // true once begin() opens logFile_ successfully; gates loop()'s recovery retries
    File logFile_;
    String csvPath_;

    bool debugEnabled_ = false;
    bool debugEverEnabled_ = false; // same idea as csvEverOpened_, for debugFile_
    File debugFile_;
    String debugPath_;

    unsigned long lastFlushMs_ = 0;
    unsigned long lastRecoveryAttemptMs_ = 0;
    LineCallback lineCallback_ = nullptr;
};

// RAII helper -- pauses Logging's own open SD files for the scope's
// lifetime (via suspend()/resume() above), so you can't forget to resume().
// Wrap ANY of your own SD.open() calls (reading or writing) in one of
// these first, including from a plugin:
//
//   { LoggingPause pause(logging); File f = SD.open("/my_plugin.csv", FILE_WRITE); ... }
//
// See the class comment on Logging above for why this is required.
class LoggingPause {
public:
    explicit LoggingPause(Logging& logging) : logging_(logging) { logging_.suspend(); }
    ~LoggingPause() { logging_.resume(); }
    LoggingPause(const LoggingPause&) = delete;
    LoggingPause& operator=(const LoggingPause&) = delete;

private:
    Logging& logging_;
};

// The single global Logging instance (defined alongside the rest of
// CarCANdy's global objects in main.cpp). Declared here so any module can
// route its diagnostic output through logging.print()/println()/printf()
// instead of calling Serial directly -- doing so is what makes that output
// show up in debug.log when debug logging is enabled. Plugins also use
// this (via LoggingPause) to safely access the SD card -- see PluginBase.h.
extern Logging logging;
