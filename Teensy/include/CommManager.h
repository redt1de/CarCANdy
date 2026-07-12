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

private:
    void handlePacket(const CarCANdy::Packet& pkt);
    bool handleCmd(const char* cmd);
    void sendConfigData();
    void sendAck();
    void sendNack();

    ConfigManager&        config_;
    CANManager&           canManager_;
    FilterManager&        filterManager_;
    CarCANdy::CommClient* client_ = nullptr;
    UARTClient             uartClient_;
    SPIClient              spiClient_;
};
