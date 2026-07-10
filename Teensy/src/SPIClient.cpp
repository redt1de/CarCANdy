#include "SPIClient.h"

bool SPIClient::begin() {
    Serial.println("[SPIClient] SPI slave mode not yet implemented -- see DOC/open-risks.md #1");
    return false;
}

bool SPIClient::available() {
    return false;
}

bool SPIClient::sendPacket(const CarCANdy::Packet&) {
    return false;
}

bool SPIClient::readPacket(CarCANdy::Packet&) {
    return false;
}

void SPIClient::poll() {
    // no-op stub
}
