#pragma once
#include "PluginBase.h"

// Stub proving the plugin pattern -- no real RPM/GPIO logic yet.
class ShiftLightPlugin : public PluginBase {
public:
    const char* getName() const override { return "shiftLight"; }
    void init(ConfigManager& config) override;
    void loop() override;
    void onCanFrame(const CarCANdy::CanFrame& frame) override;

private:
    uint16_t rpmThreshold_ = 6500;
};
