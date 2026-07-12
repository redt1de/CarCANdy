#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "CanTypes.h"

// Teensy <-> ESP32 wire protocol. See DOC/protocol.md for the full writeup,
// including the UART-resync-vs-SPI-transaction framing distinction and the
// PACKET_MAX_PAYLOAD sizing limitation (open-risks.md #2).

namespace CarCANdy {

constexpr uint8_t  PACKET_SYNC_BYTE   = 0xAA;
// Bumped 512 -> 1024 to give the config JSON (~505 bytes with the current
// plugin set, see DOC/config.example.json) real headroom instead of
// sitting right at the old ceiling. Still just correct sizing, not
// fragmentation -- a config schema that outgrew this would need real
// chunking or the other options in DOC/open-risks.md #2.
constexpr uint16_t PACKET_MAX_PAYLOAD = 1024;

enum class MsgType : uint8_t {
    PING        = 0x01,
    ACK         = 0x02,
    NACK        = 0x03,
    CMD         = 0x10,   // generic command (e.g. request restart, request status)
    CONFIG_GET  = 0x20,   // ESP32 -> Teensy: "send me current config"
    CONFIG_SET  = 0x21,   // ESP32 -> Teensy: "apply this config" (JSON bytes as payload)
    CONFIG_DATA = 0x22,   // Teensy -> ESP32: config JSON bytes (reply to CONFIG_GET)
    CAN_FRAME   = 0x30,   // Teensy -> ESP32: one CanFrame struct
    LOG_LINE    = 0x31,   // Teensy -> ESP32: text log line for status page
    STATUS      = 0x40,   // Teensy -> ESP32: periodic health/status blob
};

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  sync;    // always PACKET_SYNC_BYTE
    uint8_t  type;    // MsgType
    uint16_t length;  // payload length in bytes, 0..PACKET_MAX_PAYLOAD
};

struct Packet {
    PacketHeader header;
    uint8_t      payload[PACKET_MAX_PAYLOAD];
    uint8_t      crc8;   // computed over type + length + payload[0..length)
};
#pragma pack(pop)

// Computes the CRC8 used to validate a packet (poly/impl: simple XOR-shift,
// good enough for a wired link between two boards we control).
// Defined inline since Shared/ is a header-only library included by both
// the Teensy and ESP32 PlatformIO projects.
inline uint8_t computeCrc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07) : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

// CRC8 over type + length + payload[0..length), shared by encode/decode/
// the streaming parser so the "what gets CRC'd" definition lives in one place.
inline uint8_t packetCrc(uint8_t type, uint16_t length, const uint8_t* payload) {
    uint8_t crcBuf[1 + sizeof(uint16_t) + PACKET_MAX_PAYLOAD];
    crcBuf[0] = type;
    memcpy(crcBuf + 1, &length, sizeof(length));
    memcpy(crcBuf + 1 + sizeof(length), payload, length);
    return computeCrc8(crcBuf, 1 + sizeof(length) + length);
}

// Fills outBuf with a framed packet (header + payload + crc8). Returns the
// number of bytes written, or 0 if it doesn't fit in outBufLen.
inline size_t encodePacket(MsgType type, const uint8_t* payload, uint16_t len,
                            uint8_t* outBuf, size_t outBufLen) {
    const size_t total = sizeof(PacketHeader) + len + sizeof(uint8_t);
    if (len > PACKET_MAX_PAYLOAD || total > outBufLen) {
        return 0;
    }

    PacketHeader header{PACKET_SYNC_BYTE, static_cast<uint8_t>(type), len};
    memcpy(outBuf, &header, sizeof(PacketHeader));
    memcpy(outBuf + sizeof(PacketHeader), payload, len);
    outBuf[sizeof(PacketHeader) + len] = packetCrc(header.type, header.length, payload);

    return total;
}

// Validates a buffer that already starts at a sync byte and contains a full
// frame (used for transaction-based transports like SPI, where a fixed-size
// transfer arrives all at once). For byte-stream transports like UART, use
// PacketStreamParser instead -- a single sync byte can appear inside real
// CAN payload data, so a stream receiver can't just look for 0xAA in an
// arbitrary buffer offset.
inline bool decodePacket(const uint8_t* inBuf, size_t inLen, Packet& out) {
    if (inLen < sizeof(PacketHeader) + sizeof(uint8_t)) {
        return false;
    }
    memcpy(&out.header, inBuf, sizeof(PacketHeader));
    if (out.header.sync != PACKET_SYNC_BYTE || out.header.length > PACKET_MAX_PAYLOAD) {
        return false;
    }
    const size_t total = sizeof(PacketHeader) + out.header.length + sizeof(uint8_t);
    if (total > inLen) {
        return false;
    }
    memcpy(out.payload, inBuf + sizeof(PacketHeader), out.header.length);
    const uint8_t crc = inBuf[sizeof(PacketHeader) + out.header.length];
    return crc == packetCrc(out.header.type, out.header.length, out.payload);
}

// Incremental byte-at-a-time parser for stream transports (UART). Handles
// the false-sync case (0xAA appearing inside real CAN payload bytes) by
// validating CRC before accepting a frame; on CRC mismatch it resets and
// resumes scanning from the next byte rather than assuming the whole link
// is corrupted. See DOC/protocol.md for the framing rationale.
class PacketStreamParser {
public:
    // Feed one incoming byte. Returns true when `packet()` holds a newly
    // completed, CRC-valid packet.
    bool feed(uint8_t byte) {
        switch (state_) {
        case State::WAIT_SYNC:
            if (byte == PACKET_SYNC_BYTE) {
                packet_.header.sync = byte;
                state_ = State::READ_TYPE;
            }
            return false;

        case State::READ_TYPE:
            packet_.header.type = byte;
            state_ = State::READ_LEN_LO;
            return false;

        case State::READ_LEN_LO:
            packet_.header.length = byte;
            state_ = State::READ_LEN_HI;
            return false;

        case State::READ_LEN_HI:
            packet_.header.length = static_cast<uint16_t>(packet_.header.length | (static_cast<uint16_t>(byte) << 8));
            if (packet_.header.length > PACKET_MAX_PAYLOAD) {
                reset();
                return false;
            }
            payloadIndex_ = 0;
            state_ = packet_.header.length == 0 ? State::READ_CRC : State::READ_PAYLOAD;
            return false;

        case State::READ_PAYLOAD:
            packet_.payload[payloadIndex_++] = byte;
            if (payloadIndex_ >= packet_.header.length) {
                state_ = State::READ_CRC;
            }
            return false;

        case State::READ_CRC: {
            packet_.crc8 = byte;
            const bool valid = byte == packetCrc(packet_.header.type, packet_.header.length, packet_.payload);
            reset();
            return valid;
        }
        }
        return false;
    }

    const Packet& packet() const { return packet_; }

private:
    enum class State { WAIT_SYNC, READ_TYPE, READ_LEN_LO, READ_LEN_HI, READ_PAYLOAD, READ_CRC };

    void reset() {
        state_ = State::WAIT_SYNC;
        payloadIndex_ = 0;
    }

    State    state_ = State::WAIT_SYNC;
    Packet   packet_{};
    uint16_t payloadIndex_ = 0;
};

} // namespace CarCANdy
