#include "SPIClient.h"
#include "Logging.h"

bool SPIClient::begin() {
    logging.print("[SPIClient] SPI slave mode not yet implemented -- see DOC/open-risks.md #1\n");
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
