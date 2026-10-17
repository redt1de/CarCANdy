#include "Simulator.h"
#include "ConfigManager.h"
#include "FilterManager.h"
#include "Logging.h"
#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

namespace {

// Parses "<bus>,<id>,<extended>,<rtr>,<dlc>,<data>" (the "SIM," prefix
// already stripped) into frame. strtoul's base 16 accepts "0x204" and
// "204" alike. Mutates `fields` in place (strtok).
bool parseFields(char *fields, CarCANdy::CanFrame &frame) {
  char *busStr = strtok(fields, ",");
  char *idStr = strtok(nullptr, ",");
  char *extStr = strtok(nullptr, ",");
  char *rtrStr = strtok(nullptr, ",");
  char *dlcStr = strtok(nullptr, ",");
  char *dataStr = strtok(nullptr, ",");
  if (!busStr || !idStr || !extStr || !rtrStr || !dlcStr || !dataStr) {
    return false;
  }

  frame.bus = static_cast<uint8_t>(strtoul(busStr, nullptr, 10));
  frame.id = strtoul(idStr, nullptr, 16);
  frame.extended = static_cast<uint8_t>(strtoul(extStr, nullptr, 10));
  frame.rtr = static_cast<uint8_t>(strtoul(rtrStr, nullptr, 10));
  frame.dlc = static_cast<uint8_t>(strtoul(dlcStr, nullptr, 10));
  if (frame.dlc > 8) {
    frame.dlc = 8;
  }

  memset(frame.data, 0, sizeof(frame.data));
  const size_t dataLen = strlen(dataStr);
  for (uint8_t i = 0; i < frame.dlc && (size_t)(i * 2 + 1) < dataLen; ++i) {
    char byteStr[3] = {dataStr[i * 2], dataStr[i * 2 + 1], '\0'};
    frame.data[i] = static_cast<uint8_t>(strtoul(byteStr, nullptr, 16));
  }
  return true;
}

} // namespace

void Simulator::begin(ConfigManager &config, FilterManager &filterManager, FrameCallback callback) {
  config_ = &config;
  filterManager_ = &filterManager;
  callback_ = callback;
  lineLen_ = 0;
  lastEnabled_ = config_->simulateEnabled();
  if (lastEnabled_) {
    logging.print("[Simulator] enabled -- listening on Serial for SIM, frames\n");
  }
}

void Simulator::loop() {
  const bool enabled = config_->simulateEnabled();
  if (enabled != lastEnabled_) {
    lastEnabled_ = enabled;
    logging.print(enabled ? "[Simulator] enabled -- listening on Serial for SIM, frames\n"
                           : "[Simulator] disabled\n");
    // Drop whatever partial line was buffered while the setting flipped,
    // rather than risk later splicing bytes from before/after the toggle
    // into one bogus line.
    lineLen_ = 0;
  }
  if (!enabled) {
    return;
  }

  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c != '\n') {
      if (c != '\r' && lineLen_ < sizeof(lineBuf_) - 1) {
        lineBuf_[lineLen_++] = c;
      }
      continue;
    }

    lineBuf_[lineLen_] = '\0';
    if (strncmp(lineBuf_, "SIM,", 4) == 0) {
      CarCANdy::CanFrame frame{};
      if (parseFields(lineBuf_ + 4, frame)) {
        frame.timestampMs = millis();
        // Same gate real RX goes through in CANManager.cpp's
        // dispatchFrame() -- a simulated frame is subject to the same
        // allow/deny filter a real one would be.
        if (filterManager_->shouldPass(frame.bus, frame.id) && callback_) {
          callback_(frame);
        }
      } else {
        logging.print("[Simulator] malformed SIM line, ignoring\n");
      }
    }
    lineLen_ = 0;
  }
}
