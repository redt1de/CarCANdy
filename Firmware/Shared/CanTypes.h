#pragma once
#include <stdint.h>

namespace CarCANdy {

#pragma pack(push, 1)
struct CanFrame {
    uint8_t  bus;          // 0, 1, or 2
    uint32_t id;
    uint8_t  dlc;
    uint8_t  data[8];
    uint8_t  extended;     // 0/1
    uint8_t  rtr;          // 0/1
    uint32_t timestampMs;
};

// Payload for MsgType::STATUS -- Teensy -> ESP32 periodic health blob (see
// DOC/protocol.md). Deliberately small and easy to extend with more fields
// later; sdReady is the first one, surfaced on the WebUI's Status page.
struct StatusInfo {
    uint8_t sdReady; // 0/1 -- Logging's SD card + CSV file state this boot
};
#pragma pack(pop)

} // namespace CarCANdy
