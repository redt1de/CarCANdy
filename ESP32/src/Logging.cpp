#include "Logging.h"

void Logging::begin() {
    Serial.begin(115200);
}

void Logging::info(const char* msg) {
    Serial.print("[INFO] ");
    Serial.println(msg);
}
