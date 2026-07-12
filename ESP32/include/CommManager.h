#pragma once
#include "CommClient.h"
#include "UARTClient.h"
#include "SPIClient.h"
#include "CanTypes.h"

// Selects UARTClient or SPIClient and exposes a single interface to the
// rest of the ESP32 firmware. SPI is on hold (see DOC/open-risks.md #1) --
// the project is UART-only for now, hardcoded in main.cpp, with SPI left
// wired in for whenever that resumes.
//
// On begin(), repeatedly requests the Teensy's config (CONFIG_GET) until
// it replies with CONFIG_DATA, then caches the raw JSON for WebServerManager
// to serve at /api/config. Also dispatches inbound CAN_FRAME (a frame the
// Teensy received off the bus) and LOG_LINE packets out via callbacks so
// WebServerManager can push them to WebSocket clients.
class CommManager {
public:
    using CanFrameCallback = void (*)(const CarCANdy::CanFrame& frame);
    using LogLineCallback  = void (*)(const char* line);

    explicit CommManager(const char* commMode);

    void begin();
    void poll();

    void setCanFrameCallback(CanFrameCallback cb) { canFrameCallback_ = cb; }
    void setLogLineCallback(LogLineCallback cb) { logLineCallback_ = cb; }

    bool isConfigReady() const { return configReady_; }
    // Null-terminated JSON text last received from the Teensy's
    // CONFIG_DATA reply. Only valid to read once isConfigReady() is true.
    const char* cachedConfigJson() const { return configJson_; }

    // Sends a CONFIG_SET to the Teensy (fire-and-forget -- see .cpp for why
    // this pass doesn't correlate the reply to the HTTP request that
    // triggered it). Re-requests CONFIG_GET shortly after so the cache
    // catches up once the Teensy applies and acks it.
    bool sendConfigSet(const uint8_t* json, size_t len);

    // Requests the Teensy transmit a frame (subject to its own per-bus
    // mode check). Fire-and-forget, same as sendConfigSet().
    bool sendCanFrame(const CarCANdy::CanFrame& frame);

    // Sends a CMD packet (e.g. "RELOAD_FILTERS", "SET_FILTER_OVERRIDE:none",
    // "ADD_FILTER:1,0x201") to the Teensy -- see DOC/protocol.md for the
    // grammar. Fire-and-forget, same as sendConfigSet().
    bool sendCmd(const char* cmd);

private:
    void handlePacket(const CarCANdy::Packet& pkt);
    void requestConfig();

    static constexpr unsigned long kConfigRetryIntervalMs = 2000;
    static constexpr unsigned long kConfigRefreshDelayMs = 500;

    const char*            commMode_;
    CarCANdy::CommClient*   client_ = nullptr;
    UARTClient              uartClient_;
    SPIClient                spiClient_;

    CanFrameCallback canFrameCallback_ = nullptr;
    LogLineCallback  logLineCallback_ = nullptr;

    bool          configReady_ = false;
    char          configJson_[CarCANdy::PACKET_MAX_PAYLOAD + 1] = {};
    size_t        configJsonLen_ = 0;
    unsigned long lastConfigRequestMs_ = 0;
    bool          configRefreshPending_ = false;
    unsigned long configRefreshAtMs_ = 0;
};
