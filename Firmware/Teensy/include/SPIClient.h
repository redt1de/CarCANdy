#pragma once
#include <Arduino.h>
#include "CommClient.h"

// CommClient implementation for SPI slave mode. STUB -- see
// DOC/open-risks.md #1: stock Arduino SPI.h targets master mode, and Teensy
// 4.x LPSPI slave mode needs either direct register/DMA setup or a
// community library, not the stock SPI object. UART is the working comm
// path until this gets its own spike.
class SPIClient : public CarCANdy::CommClient {
public:
    bool begin() override;
    bool available() override;
    bool sendPacket(const CarCANdy::Packet& pkt) override;
    bool readPacket(CarCANdy::Packet& outPkt) override;
    void poll() override;
};
