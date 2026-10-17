#pragma once
#include <FlexCAN_T4.h>
#include "CanTypes.h"
#include "ConfigManager.h"

class FilterManager;

// Owns the 3 physical CAN buses. FlexCAN_T4 objects are templated on the
// CAN port, so all 3 are always statically instantiated regardless of the
// `enabled` config flag -- that flag only gates whether begin()/events() get
// called for a given bus. Per-bus `mode` ("read_only" vs "transmit") gates
// whether sendFrame() is allowed on that bus.
//
// Bus numbers are 1-3, matching the CAN controller they refer to (bus 1 =
// can1_/CAN1, bus 2 = can2_/CAN2, bus 3 = can3_/CAN3) -- 0 is never a valid
// bus number. This matches config.json's canBuses[].id and every CanFrame's
// .bus field throughout the rest of the system (logging, the ESP32 stream,
// plugins, the WebUI).
//
// Every received frame is checked against FilterManager::shouldPass()
// before the frame callback fires -- this is the single, earliest-possible
// filter point, so a dropped frame never reaches logging, the ESP32
// stream, or any plugin.
class CANManager {
public:
    using FrameCallback = void (*)(const CarCANdy::CanFrame& frame);

    CANManager(ConfigManager& config, FilterManager& filterManager);

    void begin();
    void loop();
    void setFrameCallback(FrameCallback cb);

    // Transmits a frame on the given bus (1-3). Returns false if the bus
    // is disabled, not configured for "transmit" mode, or the bus number
    // is invalid.
    bool sendFrame(uint8_t bus, const CarCANdy::CanFrame& frame);

private:
    static constexpr uint8_t kBusCount = 3;

    ConfigManager& config_;
    FrameCallback  frameCallback_ = nullptr;

    // Indexed directly by bus number (1-3); index 0 is unused.
    bool busEnabled_[kBusCount + 1] = {};
    bool busTransmitAllowed_[kBusCount + 1] = {};

    FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1_;
    FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2_;
    FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> can3_;
};
