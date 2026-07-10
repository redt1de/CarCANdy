#pragma once
#include <Arduino.h>

// Debug/status logging over the default USB Serial -- Serial2 (GPIO16/17)
// is reserved for the UART link to the Teensy when running in UART comm mode.
class Logging {
public:
    void begin();
    void info(const char* msg);
};
