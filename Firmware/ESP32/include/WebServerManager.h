#pragma once
#include <ESPAsyncWebServer.h>
#include "CommManager.h"
#include "CanTypes.h"

// Serves the WebUI static bundle from LittleFS, plus:
// - GET  /api/status  -- AP client count, comm link config-ready state, uptime
// - GET  /api/config  -- cached config JSON last fetched from the Teensy
// - POST /api/config  -- forwards a new config JSON to the Teensy (fire-and-forget,
//                         see CommManager::sendConfigSet())
// - POST /api/can/send -- {"bus":0,"id":291,"dlc":8,"data":[...]} transmit request
// - POST /api/cmd     -- {"cmd":"RELOAD_FILTERS"} etc, forwarded as a CMD packet
//                         (see DOC/protocol.md for the filter-control grammar)
// - WS   /ws          -- batched can_frames + log broadcast for Gauges/Sniffer (see queueCanFrame() below)
class WebServerManager {
public:
    explicit WebServerManager(CommManager& commManager);

    bool begin();

    // Buffers a frame for the next batched flush (see loop()) rather than
    // sending a WebSocket message immediately -- one WS message per CAN
    // frame overwhelms AsyncWebSocket's WS_MAX_QUEUED_MESSAGES (32) cap
    // under real bus load (easily hundreds of frames/sec), which silently
    // discards nearly everything once the queue is full. See
    // DOC/open-risks.md.
    void queueCanFrame(const CarCANdy::CanFrame& frame);

    // Call every main loop() iteration -- flushes any queued frames as one
    // batched WS message every kFlushIntervalMs.
    void loop();

    void broadcastLogLine(const char* line);

private:
    void flushCanFrames();

    static constexpr size_t        kMaxBatch = 64;
    static constexpr unsigned long kFlushIntervalMs = 50;

    CommManager&    commManager_;
    AsyncWebServer  server_{80};
    AsyncWebSocket  ws_{"/ws"};

    CarCANdy::CanFrame pendingFrames_[kMaxBatch];
    size_t             pendingCount_ = 0;
    unsigned long      lastFlushMs_ = 0;
};
