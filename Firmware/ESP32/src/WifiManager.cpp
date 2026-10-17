#include "WifiManager.h"
#include <WiFi.h>

bool WifiManager::begin(const char* ssid, const char* password) {
    WiFi.mode(WIFI_AP);
    return WiFi.softAP(ssid, password);
}

IPAddress WifiManager::apIP() const {
    return WiFi.softAPIP();
}
