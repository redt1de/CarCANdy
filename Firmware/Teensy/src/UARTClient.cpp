#include "UARTClient.h"

using namespace CarCANdy;

namespace {
constexpr unsigned long kBaud = 921600;
}

bool UARTClient::begin() {
    Serial2.begin(kBaud);
    return true;
}

bool UARTClient::available() {
    return queueCount_ > 0;
}

bool UARTClient::sendPacket(const Packet& pkt) {
    uint8_t buf[sizeof(PacketHeader) + PACKET_MAX_PAYLOAD + 1];
    size_t n = encodePacket(static_cast<MsgType>(pkt.header.type), pkt.payload, pkt.header.length, buf, sizeof(buf));
    if (n == 0) {
        return false;
    }
    Serial2.write(buf, n);
    return true;
}

bool UARTClient::readPacket(Packet& outPkt) {
    if (queueCount_ == 0) {
        return false;
    }
    outPkt = queue_[queueHead_];
    queueHead_ = (queueHead_ + 1) % kQueueSize;
    --queueCount_;
    return true;
}

void UARTClient::poll() {
    while (Serial2.available() > 0) {
        const uint8_t byte = Serial2.read();
        if (!parser_.feed(byte)) {
            continue;
        }
        if (queueCount_ >= kQueueSize) {
            // Queue full because CommManager isn't draining fast enough --
            // drop the new packet rather than overwrite an unread one.
            continue;
        }
        const uint8_t tail = (queueHead_ + queueCount_) % kQueueSize;
        queue_[tail] = parser_.packet();
        ++queueCount_;
    }
}
