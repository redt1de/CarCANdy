#include "ShiftLightPlugin.h"
#include <ArduinoJson.h>
#include "ConfigManager.h"

void ShiftLightPlugin::init(ConfigManager& config) {
    JsonObjectConst cfg = config.pluginConfig(getName());
    rpmThreshold_ = cfg["rpmThreshold"] | 6500;
}

void ShiftLightPlugin::loop() {
    // TODO(future): drive a shift light GPIO output based on latest RPM.
}

void ShiftLightPlugin::onCanFrame(const CarCANdy::CanFrame& frame) {
    // TODO(future): check frame.id against the RPM PID for this vehicle,
    // compare against rpmThreshold_, and drive the shift light GPIO.
    (void)frame;
}
