#include <Arduino.h>
#include "ConfigManager.h"
#include "Logging.h"
#include "CANManager.h"
#include "CommManager.h"
#include "PluginManager.h"
#include "plugins/ShiftLightPlugin.h"

ConfigManager config;
Logging logging;
CANManager canManager(config);
CommManager commManager(config, canManager);
PluginManager pluginManager(config);
ShiftLightPlugin shiftLightPlugin;

void onCanFrame(const CarCANdy::CanFrame& frame) {
    logging.logCanFrame(frame);
    commManager.streamCanFrame(frame);
    pluginManager.dispatchCanFrame(frame);
}

void setup() {
    Serial.begin(115200);

    config.begin();
    logging.begin(config.loggingPrefix());
    canManager.setFrameCallback(onCanFrame);
    canManager.begin();
    commManager.begin();

    pluginManager.registerPlugin(&shiftLightPlugin);
    pluginManager.begin();

    logging.info("CarCANdy Teensy firmware started");
}

void loop() {
    canManager.loop();
    commManager.poll();
    pluginManager.loop();
    logging.loop();
}
