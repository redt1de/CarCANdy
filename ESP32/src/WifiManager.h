#pragma once
#include <Arduino.h>

// Starts the ESP32 as a WiFi access point for the web UI.
class WifiManager {
public:
    bool begin(const char* ssid = "CarCANdy", const char* password = "carcandy1");
    IPAddress apIP() const;
};
