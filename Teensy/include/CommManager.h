#pragma once
#include "CommClient.h"
#include "ConfigManager.h"
#include "UARTClient.h"
#include "SPIClient.h"
#include "CanTypes.h"

class CANManager;
class FilterManager;

// Selects UARTClient or SPIClient based on config.commMode() and exposes a
// single interface to the rest of the Teensy firmware. Dispatches inbound
// packets from the ESP32: PING (-> ACK), CONFIG_GET (-> CONFIG_DATA reply),
// CONFIG_SET (-> apply + save + ACK/NACK), CAN_FRAME (-> transmit request on
// the named bus, gated by CANManager::sendFrame()'s mode check), and CMD
// (-> filter control, see handleCmd()/DOC/protocol.md for the grammar).
class CommManager {
public:
    CommManager(ConfigManager& config, CANManager& canManager, FilterManager& filterManager);

    void begin();
    void poll();
    void streamCanFrame(const CarCANdy::CanFrame& frame);

    // Forwards one already-formatted line (as produced by Logging's
    // print()/println()/printf(), via its line callback -- see main.cpp)
    // to the ESP32 as a LOG_LINE packet, for the WebUI's Logging page.
    // Null-guarded like streamCanFrame(), so it's safe to wire up before
    // begin() has run (Logging emits lines from very early in boot).
    void sendLogLine(const char* line, size_t len);

private:
    void handlePacket(const CarCANdy::Packet& pkt);
    bool handleCmd(const char* cmd);
    void sendConfigData();
    void sendAck();
    void sendNack();
    void sendStatus();

    static constexpr unsigned long kStatusIntervalMs = 2000;

    ConfigManager&        config_;
    CANManager&           canManager_;
    FilterManager&        filterManager_;
    CarCANdy::CommClient* client_ = nullptr;
    UARTClient             uartClient_;
    SPIClient              spiClient_;
    unsigned long          lastStatusMs_ = 0;
};
