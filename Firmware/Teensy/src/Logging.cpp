#include "Logging.h"
#include <string.h>

Logging logging;

bool Logging::begin(const char *prefix, bool debugEnabled) {
  sdReady_ = SD.begin(BUILTIN_SDCARD);
  if (!sdReady_) {
    Serial.println("[Logging] SD card not available, logging to Serial only");
    return false;
  }

  // Idempotent -- fine to call whether or not it already exists (e.g. the
  // user created it by hand ahead of time).
  SD.mkdir(kLogDir);

  const int index = findNextIndex(prefix, ".csv");
  pruneOldLogs(prefix, ".csv", index);

  char path[64];
  snprintf(path, sizeof(path), "%s/%s_%02d.csv", kLogDir, prefix, index);
  csvPath_ = path;

  logFile_ = SD.open(path, FILE_WRITE);
  if (!logFile_) {
    Serial.printf("[Logging] failed to open %s\n", path);
    sdReady_ = false;
    return false;
  }
  csvEverOpened_ = true;
  logFile_.println("timestampMs,bus,id,extended,rtr,dlc,data");
  logFile_.flush();
  Serial.printf("[Logging] logging to %s\n", path);

  // debug.log shares the CAN CSV's index (not an independent scan) so the
  // two always line up for a given boot, e.g. mustang_03.csv <-> debug_03.log.
  if (debugEnabled) {
    pruneOldLogs("debug", ".log", index);
    char debugPath[32];
    snprintf(debugPath, sizeof(debugPath), "%s/debug_%02d.log", kLogDir, index);
    debugPath_ = debugPath;
    debugFile_ = SD.open(debugPath, FILE_WRITE);
    if (debugFile_) {
      debugEnabled_ = true;
      debugEverEnabled_ = true;
      printf("[Logging] debug logging to %s\n", debugPath);
    } else {
      Serial.printf("[Logging] failed to open %s, debug logging disabled\n", debugPath);
    }
  }

  lastFlushMs_ = millis();
  return true;
}

int Logging::findNextIndex(const String &prefix, const char *extension) const {
  File root = SD.open(kLogDir);
  if (!root) {
    return 0;
  }

  const String wantPrefix = prefix + "_";
  const size_t extLen = strlen(extension);
  int maxIndex = -1;
  for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.startsWith(wantPrefix) && name.endsWith(extension)) {
        const int idx =
            name.substring(wantPrefix.length(), name.length() - extLen).toInt();
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

void Logging::pruneOldLogs(const String &prefix, const char *extension, int newIndex) const {
  // Keep only the kMaxLogFiles most recent indices, i.e.
  // [newIndex - kMaxLogFiles + 1 .. newIndex]; delete anything older.
  const int keepFromIndex = newIndex - kMaxLogFiles + 1;
  if (keepFromIndex <= 0) {
    return; // fewer than kMaxLogFiles have ever been created
  }

  File root = SD.open(kLogDir);
  if (!root) {
    return;
  }

  const String wantPrefix = prefix + "_";
  const size_t extLen = strlen(extension);
  for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.startsWith(wantPrefix) && name.endsWith(extension)) {
        const int idx =
            name.substring(wantPrefix.length(), name.length() - extLen).toInt();
        if (idx < keepFromIndex) {
          const String path = String(kLogDir) + "/" + name;
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

void Logging::suspend() {
  if (sdReady_ && logFile_) {
    logFile_.flush();
    logFile_.close();
  }
  if (debugEnabled_ && debugFile_) {
    debugFile_.flush();
    debugFile_.close();
  }
}

File Logging::reopenWithRetry(const String &path) const {
  for (uint8_t attempt = 0; attempt < kReopenAttempts; ++attempt) {
    File f = SD.open(path.c_str(), FILE_WRITE);
    if (f) {
      return f;
    }
    delay(kReopenRetryDelayMs);
  }
  return File(); // invalid -- caller checks truthiness
}

void Logging::resume() {
  // FILE_WRITE opens in append mode (seeks to end), not truncate -- these
  // pick back up where suspend() left off rather than starting over.
  //
  // This reopen has been observed to occasionally fail on real hardware
  // (right after suspend()'s close()) even though the original open in
  // begin() succeeded. Previously that left sdReady_/debugEnabled_
  // reporting true forever while logFile_/debugFile_ were silently dead --
  // CSV/debug logging would go quiet for the rest of the boot after the
  // very first config save (PluginManager::begin() triggers one on every
  // boot), with nothing -- not even the status LED -- able to tell,
  // because it only checks a one-time snapshot from setup(). Now: retry a
  // few times, and on persistent failure mark that subsystem down for
  // real, so sdReady() goes false live and loop() below can try to
  // recover later instead of staying stuck for the rest of the boot.
  if (sdReady_ && csvPath_.length() > 0) {
    logFile_ = reopenWithRetry(csvPath_);
    if (!logFile_) {
      sdReady_ = false;
      Serial.printf("[Logging] failed to reopen %s -- CSV logging stopped\n", csvPath_.c_str());
    }
  }
  if (debugEnabled_ && debugPath_.length() > 0) {
    debugFile_ = reopenWithRetry(debugPath_);
    if (!debugFile_) {
      debugEnabled_ = false;
      Serial.printf("[Logging] failed to reopen %s -- debug logging stopped\n", debugPath_.c_str());
    }
  }
}

void Logging::loop() {
  const unsigned long now = millis();

  // Self-healing: if CSV or debug logging died (resume() above gave up,
  // or some other transient fault), periodically retry rather than
  // staying down for the rest of the boot -- the original SD fault may
  // well have cleared by now.
  if (!sdReady_ && csvEverOpened_ && now - lastRecoveryAttemptMs_ >= kRecoveryIntervalMs) {
    lastRecoveryAttemptMs_ = now;
    logFile_ = reopenWithRetry(csvPath_);
    if (logFile_) {
      sdReady_ = true;
      printf("[Logging] recovered -- resumed CSV logging to %s\n", csvPath_.c_str());
    }
  }
  if (!debugEnabled_ && debugEverEnabled_ && now - lastRecoveryAttemptMs_ >= kRecoveryIntervalMs) {
    debugFile_ = reopenWithRetry(debugPath_);
    if (debugFile_) {
      debugEnabled_ = true;
      printf("[Logging] recovered -- resumed debug logging to %s\n", debugPath_.c_str());
    }
  }

  if (!sdReady_) {
    return;
  }
  if (now - lastFlushMs_ >= kFlushIntervalMs) {
    logFile_.flush();
    if (debugEnabled_ && debugFile_) {
      debugFile_.flush();
    }
    lastFlushMs_ = now;
  }
}

void Logging::info(const char *msg) {
  // One printf() call -- one write(buffer, size) call for the whole line,
  // not fragmented across multiple write() calls (print()+println() would
  // do 3 separate writes: "[INFO] ", msg, then the bare newline). That
  // matters because write() below fires lineCallback_ once per call,
  // expecting each call to already be one complete line.
  printf("[INFO] %s\n", msg);
}

void Logging::logCanFrame(const CarCANdy::CanFrame &frame) {
  char hex[17];
  const uint8_t dlc = frame.dlc > 8 ? 8 : frame.dlc;
  for (uint8_t i = 0; i < dlc; ++i) {
    snprintf(hex + i * 2, 3, "%02X", frame.data[i]);
  }
  hex[dlc * 2] = '\0';
  // Matches the SD CSV row (timestampMs,bus,id,extended,rtr,dlc,data)
  // exactly -- so the "candb live" CLI can parse this stream with the same
  // CSV parser it uses for SD log files. Only the "[CAN] " prefix differs,
  // which lets it be picked out from [INFO]/[Logging]/[CANManager] lines
  // sharing this same Serial stream. Deliberately calls Serial directly
  // (not write()/printf() above) so CAN frames are never duplicated into
  // debug.log -- see the class comment in Logging.h.
  Serial.printf("[CAN] %lu,%u,0x%03lX,%u,%u,%u,%s\n", frame.timestampMs,
                frame.bus, frame.id, frame.extended, frame.rtr, frame.dlc, hex);

  if (!sdReady_) {
    return;
  }
  logFile_.printf("%lu,%u,0x%03lX,%u,%u,%u,", frame.timestampMs, frame.bus,
                  frame.id, frame.extended, frame.rtr, frame.dlc);
  for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
    logFile_.printf("%02X", frame.data[i]);
  }
  logFile_.println();
  // Not flushed here -- see loop() for the periodic flush strategy.
}

size_t Logging::write(uint8_t c) {
  Serial.write(c);
  if (debugEnabled_ && debugFile_) {
    debugFile_.write(c);
  }
  return 1;
}

size_t Logging::write(const uint8_t *buffer, size_t size) {
  Serial.write(buffer, size);
  if (debugEnabled_ && debugFile_) {
    debugFile_.write(buffer, size);
  }
  if (lineCallback_) {
    lineCallback_(reinterpret_cast<const char *>(buffer), size);
  }
  return size;
}
