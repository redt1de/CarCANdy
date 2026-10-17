#pragma once
#include "Packet.h"

// Abstract interface implemented identically by UARTClient and SPIClient on
// both the Teensy and ESP32 projects, so CommManager can swap transports
// without changing any calling code. See DOC/open-risks.md for the current
// status of the Teensy-side SPI slave implementation.

namespace CarCANdy {

class CommClient {
public:
    virtual ~CommClient() = default;

    virtual bool begin() = 0;
    virtual bool available() = 0;
    virtual bool sendPacket(const Packet& pkt) = 0;
    virtual bool readPacket(Packet& outPkt) = 0;
    virtual void poll() = 0;
};

} // namespace CarCANdy
