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
#pragma pack(pop)

} // namespace CarCANdy
