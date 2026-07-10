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
// - WS   /ws          -- live CAN_FRAME/LOG_LINE broadcast for Gauges/Sniffer (Stage 3)
class WebServerManager {
public:
    explicit WebServerManager(CommManager& commManager);

    bool begin();
    void broadcastCanFrame(const CarCANdy::CanFrame& frame);
    void broadcastLogLine(const char* line);

private:
    CommManager&    commManager_;
    AsyncWebServer  server_{80};
    AsyncWebSocket  ws_{"/ws"};
};
