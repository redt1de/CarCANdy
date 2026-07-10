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

ESP32 board is a classic ESP32 (`esp32dev`, e.g. WROOM-32 devkit), not
ESP32-S3.

| Teensy Pin | Function              | Wire Color | Connects To (ESP32) | Notes |
|------------|-----------------------|------------|----------------------|-------|
| 8 (TX2)    | UART TX (Serial2)     |            | ESP32 GPIO16 (RX2)   | UART mode -- Teensy Serial1 avoided, its pins are shared with CAN1 |
| 7 (RX2)    | UART RX (Serial2)     |            | ESP32 GPIO17 (TX2)   | UART mode -- Teensy Serial1 avoided, its pins are shared with CAN1 |
| ?          | SPI MISO (slave out)  |            | ESP32 SPI MISO       | SPI mode (paused, see open-risks.md) |
| ?          | SPI MOSI (slave in)   |            | ESP32 SPI MOSI       | SPI mode (paused, see open-risks.md) |
| ?          | SPI SCK               |            | ESP32 SPI SCK        | SPI mode (paused, see open-risks.md) |
| ?          | SPI CS                |            | ESP32 SPI CS         | SPI mode (paused, see open-risks.md) |
| ?          | DRDY (out)            |            | ESP32 GPIO (in, interrupt) | Signals Teensy has data queued (SPI mode only) |
| GND        | Common ground         |            | ESP32 GND            | Required for both UART and SPI |

ESP32 debug/status output is on the default USB `Serial` (UART0), not
Serial2 -- Serial2 (GPIO16/17) is reserved exclusively for the Teensy link.

## SD Card

Teensy 4.1 built-in SDIO slot — no extra wiring.

## Future GPIO (buttons / switches / shift light / gauges)

| Teensy Pin | Function | Wire Color | Notes |
|------------|----------|------------|-------|
| ?          |          |            |       |
