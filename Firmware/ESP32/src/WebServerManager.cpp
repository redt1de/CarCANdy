#include "WebServerManager.h"
#include <LittleFS.h>
#include <WiFi.h>
#include <string.h>

using namespace CarCANdy;

namespace {
// Minimal JSON string escaping for broadcastLogLine() below -- a log line
// could contain a quote (e.g. a filter spec) or a stray control byte, and
// without this an unescaped one breaks the JSON for every WS client, not
// just corrupts that one line. Unescapable control bytes are dropped
// rather than emitted raw. Always null-terminates; truncates rather than
// overflowing if the escaped form doesn't fit outLen.
void escapeJsonInto(char* out, size_t outLen, const char* in) {
    size_t w = 0;
    for (; *in != '\0' && w + 1 < outLen; ++in) {
        const unsigned char c = static_cast<unsigned char>(*in);
        const char* esc = nullptr;
        switch (c) {
        case '"':  esc = "\\\""; break;
        case '\\': esc = "\\\\"; break;
        case '\n': esc = "\\n"; break;
        case '\r': esc = "\\r"; break;
        case '\t': esc = "\\t"; break;
        default:
            if (c < 0x20) {
                continue; // drop other control bytes -- not valid raw in JSON
            }
        }
        if (esc) {
            const size_t escLen = strlen(esc);
            if (w + escLen >= outLen) {
                break;
            }
            memcpy(out + w, esc, escLen);
            w += escLen;
        } else {
            out[w++] = static_cast<char>(c);
        }
    }
    out[w] = '\0';
}
} // namespace

WebServerManager::WebServerManager(CommManager& commManager) : commManager_(commManager) {}

bool WebServerManager::begin() {
    if (!LittleFS.begin()) {
        Serial.println("[WebServerManager] LittleFS mount failed");
        return false;
    }

    ws_.onEvent([](AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type, void*, uint8_t*, size_t) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("[WebServerManager] WS client #%u connected\n", client->id());
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("[WebServerManager] WS client #%u disconnected\n", client->id());
        }
    });
    server_.addHandler(&ws_);

    server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        char buf[192];
        snprintf(buf, sizeof(buf), "{\"apClients\":%u,\"configReady\":%s,\"sdReady\":%s,\"uptimeMs\":%lu}",
                 WiFi.softAPgetStationNum(), commManager_.isConfigReady() ? "true" : "false",
                 commManager_.isSdReady() ? "true" : "false", static_cast<unsigned long>(millis()));
        request->send(200, "application/json", buf);
    });

    server_.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        if (!commManager_.isConfigReady()) {
            request->send(503, "application/json", "{\"error\":\"config not yet available\"}");
            return;
        }
        request->send(200, "application/json", commManager_.cachedConfigJson());
    });

    server_.on(
        "/api/config", HTTP_POST,
        [](AsyncWebServerRequest* request) { request->send(202, "application/json", "{\"status\":\"queued\"}"); },
        nullptr,
        [this](AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total) {
            // TODO(Stage 3): handle bodies split across multiple chunks --
            // fine for a config-sized JSON payload arriving in one piece.
            if (index == 0 && total <= PACKET_MAX_PAYLOAD) {
                commManager_.sendConfigSet(data, len);
            }
        });

    server_.on(
        "/api/can/send", HTTP_POST,
        [](AsyncWebServerRequest* request) { request->send(202, "application/json", "{\"status\":\"queued\"}"); },
        nullptr,
        [this](AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index != 0 || total > 256) {
                return;
            }
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                return;
            }
            CanFrame frame{};
            frame.bus = doc["bus"] | 0;
            frame.id = doc["id"] | 0;
            frame.dlc = doc["dlc"] | 0;
            frame.extended = doc["extended"] | 0;
            frame.rtr = doc["rtr"] | 0;
            uint8_t i = 0;
            for (JsonVariantConst v : doc["data"].as<JsonArrayConst>()) {
                if (i >= 8) {
                    break;
                }
                frame.data[i++] = v.as<uint8_t>();
            }
            commManager_.sendCanFrame(frame);
        });

    server_.on(
        "/api/cmd", HTTP_POST,
        [](AsyncWebServerRequest* request) { request->send(202, "application/json", "{\"status\":\"queued\"}"); },
        nullptr,
        [this](AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index != 0 || total > 200) {
                return;
            }
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                return;
            }
            const char* cmd = doc["cmd"] | "";
            if (cmd[0] != '\0') {
                commManager_.sendCmd(cmd);
            }
        });

    server_.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server_.begin();
    return true;
}

void WebServerManager::queueCanFrame(const CanFrame& frame) {
    if (ws_.count() == 0) {
        return; // nobody listening -- don't bother buffering
    }
    if (pendingCount_ >= kMaxBatch) {
        return; // batch already full for this interval -- drop rather than
                // grow unbounded. The Teensy's SD card CSV remains the
                // complete, lossless record regardless; this stream was
                // only ever a live preview.
    }
    pendingFrames_[pendingCount_++] = frame;
}

void WebServerManager::loop() {
    if (millis() - lastFlushMs_ < kFlushIntervalMs) {
        return;
    }
    lastFlushMs_ = millis();
    flushCanFrames();
}

void WebServerManager::flushCanFrames() {
    if (pendingCount_ == 0 || ws_.count() == 0) {
        pendingCount_ = 0;
        return;
    }

    // One WS message per flush interval instead of one per frame -- see
    // queueCanFrame()'s comment for why per-frame broadcasting silently
    // drops almost everything under real bus load.
    String out;
    out.reserve(32 + pendingCount_ * 140);
    out += "{\"type\":\"can_frames\",\"frames\":[";
    for (size_t i = 0; i < pendingCount_; ++i) {
        if (i > 0) {
            out += ',';
        }
        const CanFrame& frame = pendingFrames_[i];
        char hex[17];
        const uint8_t dlc = frame.dlc > 8 ? 8 : frame.dlc;
        for (uint8_t b = 0; b < dlc; ++b) {
            snprintf(hex + b * 2, 3, "%02X", frame.data[b]);
        }
        hex[dlc * 2] = '\0';

        char buf[160];
        snprintf(buf, sizeof(buf), "{\"bus\":%u,\"id\":%lu,\"extended\":%s,\"rtr\":%s,\"dlc\":%u,\"data\":\"%s\",\"ts\":%lu}",
                 frame.bus, static_cast<unsigned long>(frame.id), frame.extended ? "true" : "false",
                 frame.rtr ? "true" : "false", dlc, hex, static_cast<unsigned long>(frame.timestampMs));
        out += buf;
    }
    out += "]}";

    ws_.textAll(out);
    pendingCount_ = 0;
}

void WebServerManager::broadcastLogLine(const char* line) {
    if (ws_.count() == 0) {
        return;
    }
    char escaped[192];
    escapeJsonInto(escaped, sizeof(escaped), line);
    char buf[224];
    snprintf(buf, sizeof(buf), "{\"type\":\"log\",\"msg\":\"%s\"}", escaped);
    ws_.textAll(buf);
}
