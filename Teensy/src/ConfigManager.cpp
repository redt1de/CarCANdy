#include "ConfigManager.h"
#include <SD.h>

namespace {
constexpr const char *kConfigPath = "/config.json";
}

bool ConfigManager::begin() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("[Config] SD card not available, using defaults");
    loadDefaults();
    return false;
  }

  File f = SD.open(kConfigPath, FILE_READ);
  if (!f) {
    Serial.println("[Config] /config.json not found, using defaults");
    loadDefaults();
    return false;
  }

  DeserializationError err = deserializeJson(doc_, f);
  f.close();
  if (err) {
    Serial.print("[Config] parse failed: ");
    Serial.println(err.c_str());
    loadDefaults();
    return false;
  }

  return true;
}

bool ConfigManager::save() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("[Config] save() failed: SD card not available");
    return false;
  }
  if (SD.exists(kConfigPath)) {
    SD.remove(kConfigPath);
  }
  File f = SD.open(kConfigPath, FILE_WRITE);
  if (!f) {
    Serial.println(
        "[Config] save() failed: could not open /config.json for writing");
    return false;
  }
  serializeJson(doc_, f);
  f.close();
  return true;
}

bool ConfigManager::applyJson(const uint8_t *data, size_t len) {
  JsonDocument newDoc;
  DeserializationError err = deserializeJson(newDoc, data, len);
  if (err) {
    Serial.print("[Config] applyJson() parse failed: ");
    Serial.println(err.c_str());
    return false;
  }
  doc_ = newDoc;
  return save();
}

size_t ConfigManager::toJson(char *outBuf, size_t outBufLen) const {
  const size_t n = serializeJson(doc_, outBuf, outBufLen);
  // serializeJson() stops early if the buffer is too small; treat filling
  // the whole buffer as a truncation signal rather than a clean fit.
  return n >= outBufLen ? 0 : n;
}

void ConfigManager::loadDefaults() {
  doc_.clear();
  doc_["commMode"] = "UART";

  JsonArray buses = doc_["canBuses"].to<JsonArray>();
  struct {
    uint8_t id;
    bool enabled;
    uint32_t baud;
    const char *mode;
  } defaults[] = {
      {0, true, 500000, "read_only"},
      {1, true, 500000, "read_only"},
      {2, false, 500000, "read_only"},
  };
  for (auto &d : defaults) {
    JsonObject b = buses.add<JsonObject>();
    b["id"] = d.id;
    b["enabled"] = d.enabled;
    b["baud"] = d.baud;
    b["mode"] = d.mode;
  }

  JsonObject logging = doc_["logging"].to<JsonObject>();
  logging["toSerial"] = true;
  logging["toSD"] = true;
  logging["toComm"] = true;
  logging["prefix"] = "canlog";

  JsonObject plugins = doc_["plugins"].to<JsonObject>();
  JsonObject shiftLight = plugins["shiftLight"].to<JsonObject>();
  shiftLight["enabled"] = false;
  shiftLight["config"]["rpmThreshold"] = 6500;
}

const char *ConfigManager::commMode() const {
  return doc_["commMode"] | "UART";
}

const char *ConfigManager::loggingPrefix() const {
  return doc_["logging"]["prefix"] | "canlog";
}

ConfigManager::CanBusConfig ConfigManager::canBus(uint8_t index) const {
  JsonArrayConst buses = doc_["canBuses"];
  JsonObjectConst b = buses[index];
  return CanBusConfig{
      b["id"] | index,
      b["enabled"] | false,
      b["baud"] | 500000u,
      b["mode"] | "read_only",
  };
}

bool ConfigManager::isPluginEnabled(const char *name) const {
  return doc_["plugins"][name]["enabled"] | false;
}

JsonObjectConst ConfigManager::pluginConfig(const char *name) const {
  return doc_["plugins"][name]["config"];
}
