#pragma once
#include <Arduino.h>

// Debug/status logging over the default USB Serial -- whichever UART is
// selected for the Teensy link (see UARTClient.cpp / platformio.ini's
// CARCANDY_UART_PORT) is reserved exclusively for that when running in
// UART comm mode.
class Logging {
public:
    void begin();
    void info(const char* msg);
};
