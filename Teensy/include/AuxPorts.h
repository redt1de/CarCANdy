#pragma once
#include <Arduino.h>

// Named pins for CarCANdy's 2 auxiliary ports, so plugins don't have to
// hardcode/memorize which Teensy pin number is wired to which port. Each
// port is broken out with 5V, GND, and 2 signal pins; every signal pin
// supports digital in/out and PWM (analogWrite()), each port's pair also
// doubles as one Teensy hardware UART, and AUX2's pair is additionally
// wired to the ADC. A pin can only be used one way at a time -- pick
// whichever constant/object matches the mode a plugin needs and drive it
// with the normal Arduino API (pinMode/digitalWrite/digitalRead/
// analogWrite/analogRead, or *_SERIAL.begin()); nothing here reconfigures a
// pin automatically. Physical wiring mirrors DOC/pinout.md.
//
//   AUX1: pin 28 = RX, pin 29 = TX  -- UART: Teensy Serial7
//   AUX2: pin 24 = TX, pin 25 = RX  -- UART: Teensy Serial6; also A10/A11
//
// Example:
//   pinMode(AuxPorts::AUX1_TX_PIN, OUTPUT);
//   digitalWrite(AuxPorts::AUX1_TX_PIN, HIGH);
//
//   AuxPorts::AUX2_SERIAL.begin(9600);
//
//   int raw = analogRead(AuxPorts::AUX2_RX_ANALOG_PIN);
namespace AuxPorts {

// ---- AUX1 -- digital, PWM, UART only ----
constexpr uint8_t AUX1_RX_PIN = 28;
constexpr uint8_t AUX1_TX_PIN = 29;
inline HardwareSerial &AUX1_SERIAL = Serial7; // matches AUX1_RX_PIN/AUX1_TX_PIN

// ---- AUX2 -- digital, PWM, UART, and analog ----
constexpr uint8_t AUX2_TX_PIN = 24;
constexpr uint8_t AUX2_RX_PIN = 25;
constexpr uint8_t AUX2_TX_ANALOG_PIN = A10; // same physical pin as AUX2_TX_PIN
constexpr uint8_t AUX2_RX_ANALOG_PIN = A11; // same physical pin as AUX2_RX_PIN
inline HardwareSerial &AUX2_SERIAL = Serial6; // matches AUX2_RX_PIN/AUX2_TX_PIN

} // namespace AuxPorts
