#include "CANManager.h"
#include "CommManager.h"
#include "ConfigManager.h"
#include "FilterManager.h"
#include "Logging.h"
#include "PluginManager.h"
#include "plugins/ShiftLightPlugin.h"
#include <Arduino.h>

ConfigManager config;
Logging logging;
FilterManager filterManager(config);
CANManager canManager(config, filterManager);
CommManager commManager(config, canManager, filterManager);
PluginManager pluginManager(config);
ShiftLightPlugin shiftLightPlugin;

void onCanFrame(const CarCANdy::CanFrame &frame) {
  logging.logCanFrame(frame);
  commManager.streamCanFrame(frame);
  pluginManager.dispatchCanFrame(frame);
}

void setup() {
  while (!Serial && millis() < 5000) { // <<<<<<<<<<<<<< REMOVE ME
    ;
  }

  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  config.begin();
  logging.begin(config.loggingPrefix());
  filterManager.begin();
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
