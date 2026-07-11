#pragma once
#include <Arduino.h>
#include "CommClient.h"

// CommClient implementation over Serial2 (the physical link to the ESP32
// when running in UART mode). Serial1 is intentionally avoided -- its pins
// are shared with CAN1 on the Teensy 4.1. Feeds incoming bytes through a
// CarCANdy::PacketStreamParser (see Shared/Packet.h) so sync-byte
// collisions with real CAN payload data don't corrupt the stream, and
// queues completed packets so a burst of several packets between poll()
// calls isn't silently dropped.
class UARTClient : public CarCANdy::CommClient {
public:
    bool begin() override;
    bool available() override;
    bool sendPacket(const CarCANdy::Packet& pkt) override;
    bool readPacket(CarCANdy::Packet& outPkt) override;
    void poll() override;

private:
    static constexpr uint8_t kQueueSize = 8;

    CarCANdy::PacketStreamParser parser_;
    CarCANdy::Packet queue_[kQueueSize];
    uint8_t queueHead_ = 0;
    uint8_t queueCount_ = 0;
};
