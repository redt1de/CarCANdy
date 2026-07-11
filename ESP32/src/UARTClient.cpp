#include "UARTClient.h"

using namespace CarCANdy;

// Board-specific UART object + pins for the Teensy link -- set via
// build_flags per PlatformIO environment (see ESP32/platformio.ini).
// Falls back to the classic ESP32 (esp32dev) Serial2/GPIO16+17 if an
// environment doesn't define these, so this still compiles standalone.
#ifndef CARCANDY_UART_PORT
#define CARCANDY_UART_PORT Serial2
#endif
#ifndef CARCANDY_UART_RX_PIN
#define CARCANDY_UART_RX_PIN 16
#endif
#ifndef CARCANDY_UART_TX_PIN
#define CARCANDY_UART_TX_PIN 17
#endif

namespace {
constexpr unsigned long kBaud = 921600;
} // namespace

bool UARTClient::begin() {
  CARCANDY_UART_PORT.begin(kBaud, SERIAL_8N1, CARCANDY_UART_RX_PIN, CARCANDY_UART_TX_PIN);
  // Confirms at runtime what this build actually compiled in -- cheap
  // insurance against a stale/wrong-environment flash being the real
  // problem instead of wiring.
  Serial.printf("[UARTClient] began on RX=%d TX=%d @ %lu baud\n", CARCANDY_UART_RX_PIN, CARCANDY_UART_TX_PIN, kBaud);
  return true;
}

bool UARTClient::available() { return queueCount_ > 0; }

bool UARTClient::sendPacket(const Packet &pkt) {
  uint8_t buf[sizeof(PacketHeader) + PACKET_MAX_PAYLOAD + 1];
  size_t n = encodePacket(static_cast<MsgType>(pkt.header.type), pkt.payload,
                          pkt.header.length, buf, sizeof(buf));
  if (n == 0) {
    return false;
  }
  CARCANDY_UART_PORT.write(buf, n);
  return true;
}

bool UARTClient::readPacket(Packet &outPkt) {
  if (queueCount_ == 0) {
    return false;
  }
  outPkt = queue_[queueHead_];
  queueHead_ = (queueHead_ + 1) % kQueueSize;
  --queueCount_;
  return true;
}

void UARTClient::poll() {
  while (CARCANDY_UART_PORT.available() > 0) {
    const uint8_t byte = CARCANDY_UART_PORT.read();
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
