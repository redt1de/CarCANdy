#include "CommManager.h"
#include "Logging.h"
#include "WebServerManager.h"
#include "WifiManager.h"
#include <Arduino.h>
Logging logging;
WifiManager wifiManager;
// SPI is on hold for now (see DOC/open-risks.md #1) -- UART only until that
// resumes.
CommManager commManager("UART");
WebServerManager webServerManager(commManager);

void onCanFrame(const CarCANdy::CanFrame &frame) {
  webServerManager.queueCanFrame(frame);
}

void onLogLine(const char *line) { webServerManager.broadcastLogLine(line); }

void setup() {
  logging.begin();
  logging.info("CarCANdy ESP32 firmware starting");

  if (wifiManager.begin()) {
    logging.info("WiFi AP started");
  } else {
    logging.info("WiFi AP failed to start");
  }

  commManager.setCanFrameCallback(onCanFrame);
  commManager.setLogLineCallback(onLogLine);
  commManager.begin();

  if (webServerManager.begin()) {
    logging.info("Web server started");
  } else {
    logging.info("Web server failed to start");
  }
}

void loop() {
  commManager.poll();
  webServerManager.loop();
}
