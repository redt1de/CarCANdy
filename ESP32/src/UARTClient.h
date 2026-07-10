#pragma once
#include <Arduino.h>
#include "CommClient.h"

// CommClient implementation over Serial2 -- GPIO16 (RX2) / GPIO17 (TX2),
// UART2's default pins on the classic ESP32 (esp32dev board) -- the
// physical link to the Teensy when running in UART mode. Serial2 is used
// exclusively for this link; all debug/status output goes over the
// default USB Serial instead (see Logging), since mixing plain-text debug
// prints into this stream would corrupt the binary packet framing. Feeds
// incoming bytes through a
// CarCANdy::PacketStreamParser (see Shared/Packet.h) so sync-byte
// collisions with real CAN payload data don't corrupt the stream, and
// queues completed packets so a burst of several packets between poll()
// calls isn't silently dropped. Mirrors Teensy/src/UARTClient.
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
