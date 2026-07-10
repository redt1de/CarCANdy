#pragma once
#include <Arduino.h>
#include "CommClient.h"

// CommClient implementation for SPI master mode -- the ESP32 polls the
// Teensy (SPI slave) and watches a DRDY GPIO interrupt line to know when
// the Teensy has a frame queued. STUB this pass: the Teensy-side slave
// implementation is an open risk (see DOC/open-risks.md #1), so this
// master-side stub has nothing to talk to yet.
class SPIClient : public CarCANdy::CommClient {
public:
    bool begin() override;
    bool available() override;
    bool sendPacket(const CarCANdy::Packet& pkt) override;
    bool readPacket(CarCANdy::Packet& outPkt) override;
    void poll() override;
};
