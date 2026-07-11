#pragma once
#include <FlexCAN_T4.h>
#include "CanTypes.h"
#include "ConfigManager.h"

// Owns the 3 physical CAN buses. FlexCAN_T4 objects are templated on the
// CAN port, so all 3 are always statically instantiated regardless of the
// `enabled` config flag -- that flag only gates whether begin()/events() get
// called for a given bus. Per-bus `mode` ("read_only" vs "transmit") gates
// whether sendFrame() is allowed on that bus.
class CANManager {
public:
    using FrameCallback = void (*)(const CarCANdy::CanFrame& frame);

    explicit CANManager(ConfigManager& config);

    void begin();
    void loop();
    void setFrameCallback(FrameCallback cb);

    // Transmits a frame on the given bus. Returns false if the bus is
    // disabled, not configured for "transmit" mode, or the bus index is
    // invalid.
    bool sendFrame(uint8_t bus, const CarCANdy::CanFrame& frame);

private:
    static constexpr uint8_t kBusCount = 3;

    ConfigManager& config_;
    FrameCallback  frameCallback_ = nullptr;

    bool busEnabled_[kBusCount] = {};
    bool busTransmitAllowed_[kBusCount] = {};

    FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1_;
    FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2_;
    FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> can3_;
};
