# Pinout / Wiring Reference

Fill this in against the actual prototype wiring. Placeholder rows below.

## Teensy 4.1 <-> SN65HVD230 transceivers

| Teensy Pin | Function       | Wire Color | Connects To      | Notes |
|------------|----------------|------------|-------------------|-------|
| ?          | CAN1 TX        |            | Transceiver 1 TXD |       |
| ?          | CAN1 RX        |            | Transceiver 1 RXD |       |
| ?          | CAN2 TX        |            | Transceiver 2 TXD |       |
| ?          | CAN2 RX        |            | Transceiver 2 RXD |       |
| ?          | CAN3 TX        |            | Transceiver 3 TXD |       |
| ?          | CAN3 RX        |            | Transceiver 3 RXD |       |
| 3.3V       | Transceiver VCC|            | All 3 transceivers|       |
| GND        | Transceiver GND|            | All 3 transceivers|       |
|            | CANH / CANL    |            | Vehicle bus(es)   |       |

## Teensy 4.1 <-> ESP32 link

Two ESP32 boards are supported, selected by PlatformIO environment (see
`ESP32/platformio.ini`) -- which UART object + pins the ESP32 side uses is
set per environment via `CARCANDY_UART_PORT`/`CARCANDY_UART_RX_PIN`/
`CARCANDY_UART_TX_PIN` build flags, read by `ESP32/src/UARTClient.cpp`.

### `esp32dev` -- classic ESP32 (e.g. WROOM-32 devkit)

| Teensy Pin | Function              | Wire Color | Connects To (ESP32) | Notes |
|------------|-----------------------|------------|----------------------|-------|
| 8 (TX2)    | UART TX (Serial2)     |            | ESP32 GPIO16 (RX2)   | UART mode -- Teensy Serial1 avoided, its pins are shared with CAN1 |
| 7 (RX2)    | UART RX (Serial2)     |            | ESP32 GPIO17 (TX2)   | UART mode -- Teensy Serial1 avoided, its pins are shared with CAN1 |
| GND        | Common ground         |            | ESP32 GND            | Required |

ESP32 debug/status output is on the default USB `Serial` (via the board's
USB-UART bridge chip). Serial2 (GPIO16/17) is reserved exclusively for the
Teensy link.

### `esp32-s3-mini` -- ESP32-S3-Mini ("Super Mini ESP32-S3" clone board, 19 pins)

This is a generic/clone board (see `DOC/ESP32-S3-Mini_pinout.jpg`), not an
official Espressif dev board -- its silkscreen "TX"/"RX" pin labels weren't
reliably identifiable, so the link intentionally uses two unambiguous
numbered GPIOs instead of trying to match those.

| Teensy Pin | Function              | Wire Color | Connects To (ESP32-S3) | Notes |
|------------|-----------------------|------------|-------------------------|-------|
| 8 (TX2)    | UART TX (Serial2)     |            | ESP32-S3 GP2 (UART2 RX, remapped) | |
| 7 (RX2)    | UART RX (Serial2)     |            | ESP32-S3 GP1 (UART2 TX, remapped) | |
| GND        | Common ground         |            | ESP32-S3 GND          | Required |

This board has no separate USB-UART bridge chip -- the USB-C port is the
ESP32-S3's native USB peripheral. `ARDUINO_USB_CDC_ON_BOOT=1` (set in
`platformio.ini`) is what makes the debug `Serial` object use that native
USB connection; without it, `Serial` would silently have no path to the
USB-C port at all. This is independent of the GP1/GP2 choice above --
UART2 is remapped onto those pins via the ESP32-S3's GPIO matrix (every
pin on this board supports UART, matching its pinout diagram's per-pin
capability tags), the same general approach `esp32dev` uses to put UART2
on GPIO16/17.

**UART1 was tried first and didn't work** on this board, even with
identical pins/baud -- confirmed via a standalone test sketch that UART2
(`HardwareSerial(2)`) works on the same GP1/GP2 pins where UART1 didn't.
No documented reservation of UART1 was found in the Arduino-ESP32 core
source, so the root cause is unconfirmed; UART2 was adopted because it's
the empirically-verified working configuration.

### SPI (both boards, paused -- see open-risks.md)

| Function              | Notes |
|------------------------|-------|
| SPI MISO (slave out)  | Teensy -- not yet wired/used |
| SPI MOSI (slave in)   | Teensy -- not yet wired/used |
| SPI SCK               | not yet wired/used |
| SPI CS                | not yet wired/used |
| DRDY (out)            | Teensy -> ESP32 GPIO (interrupt), signals a frame is queued |

## SD Card

Teensy 4.1 built-in SDIO slot — no extra wiring.

## Future GPIO (buttons / switches / shift light / gauges)

| Teensy Pin | Function | Wire Color | Notes |
|------------|----------|------------|-------|
| ?          |          |            |       |
