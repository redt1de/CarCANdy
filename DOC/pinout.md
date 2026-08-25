# Pinout / Wiring Reference




## OBD2 plug
SIGNAL      |  PIN  | COLOR
---------------------------------------
HS_CAN1_H   | 6     | BLUE
HS_CAN1_L   | 14    | WHITE/BLUE

MS_CAN_H    | 3     | GREEN
MS_CAN_L    | 11    | WHITE/GREEN

HS_CAN2_H   | 12    | BROWN
HS_CAN2_L   | 13    | WHITE/BROWN

12v+        | 16    | ORANGE
CHASSIS_GND | 4     | WHITE/ORANGE
SIGNAL_GND  | 5     | WHITE/ORANGE

# RJ45 Wiring (Gold pins facing away from you, clip on top)
        ___
-------|   |-------
|                 | 
| 8 7 6 5 4 3 2 1 |
| | | | | | | | | |
-------------------


# V1/prototype
1: CAN3_L - white/brown
2: CAN3_H - brown
3: GND - white/orange
4: CAN2_L - white/green
5: NA
6: CAN2_H - green
7: CAN1_H - blue
8: CAN1_L - white/blue


# V2 final
1: 12V/NA
2: GND - white/orange
3: CAN3_H - brown
4: CAN3_L - white/brown
5: CAN2_H - green
6: CAN2_L - white/green
7: CAN1_H - blue
8: CAN1_L - white/blue
--------------------------------


## Teensy CAN Connections
CAN1_RX: 23 
CAN1_TX: 22
CAN2_RX: 0
CAN2_TX: 1
CAN3_RX: 30
CAN3_TX: 31

Bus numbers used everywhere in the firmware/config/WebUI (config.json's
canBuses[].id, CanFrame.bus, the filter CSV's BUS column, etc.) match the
CAN controller directly: bus 1 = CAN1, bus 2 = CAN2, bus 3 = CAN3 -- there
is no bus 0. Which OBD2 signal (HS_CAN1 / MS_CAN / HS_CAN2, per the pinout
above) each transceiver is physically wired to:

| Bus | Controller | Wired to (fill in) |
|-----|------------|---------------------|
| 1   | CAN1       | ?                   |
| 2   | CAN2       | ?                   |
| 3   | CAN3       | ?                   |

| Teensy Pin | Function       | Wire Color | Connects To       | Notes |
|------------|----------------|------------|-------------------|-------|
| 22         | CAN1 TX        |            | Transceiver 1 TXD |       |
| 23         | CAN1 RX        |            | Transceiver 1 RXD |       |
| 2          | CAN2 TX        |            | Transceiver 2 TXD |       |
| 0          | CAN2 RX        |            | Transceiver 2 RXD |       |
| 31         | CAN3 TX        |            | Transceiver 3 TXD |       |
| 30         | CAN3 RX        |            | Transceiver 3 RXD |       |
| 3.3V       | Transceiver VCC|            | All 3 transceivers|       |
| GND        | Transceiver GND|            | All 3 transceivers|       |
1 
## Teensy 4.1 <-> ESP32 link

#### `esp32dev` -- classic ESP32 (e.g. WROOM-32 devkit)

| Teensy Pin | Function              | Wire Color | Connects To (ESP32) | Notes |
|------------|-----------------------|------------|----------------------|-------|
| 8 (TX2)    | UART TX (Serial2)     |            | ESP32 GPIO16 (RX2)   | UART mode -- Teensy Serial1 avoided, its pins are shared with CAN1 |
| 7 (RX2)    | UART RX (Serial2)     |            | ESP32 GPIO17 (TX2)   | UART mode -- Teensy Serial1 avoided, its pins are shared with CAN1 |
| GND        | Common ground         |            | ESP32 GND            | Required |


#### `esp32-s3-mini` -- ESP32-S3-Mini ("Super Mini ESP32-S3" clone board, 19 pins)

| Teensy Pin | Function              | Wire Color | Connects To (ESP32-S3) | Notes |
|------------|-----------------------|------------|-------------------------|-------|
| 8 (TX2)    | UART TX (Serial2)     |            | ESP32-S3 GP2 (UART2 RX, remapped) | |
| 7 (RX2)    | UART RX (Serial2)     |            | ESP32-S3 GP1 (UART2 TX, remapped) | |
| GND        | Common ground         |            | ESP32-S3 GND          | Required |



### SPI 
Teensy SPI channels:
SPI Bus    | MOSI | MISO | SCK	| CS
--------------------------------------------------
SPI (SPI0) | 11   |	12	 | 13	| 10 (or any GPIO)
SPI1	   | 26   |	1	 | 27	| 0 (or any GPIO)
SPI2	   | 43   |	42	 | 45	| 44 (or any GPIO)



## SD Card

Teensy 4.1 built-in SDIO slot — no extra wiring.

## AUX ports (buttons / switches / shift light / gauges)

2 auxiliary ports, each broken out with 5V, GND, and 2 signal pins. Both
signal pins on both ports support digital in/out and PWM; each pair also
doubles as one Teensy hardware UART; AUX2's pair is additionally wired to
the ADC (A10/A11). See `Teensy/include/AuxPorts.h` for named constants
plugins should use instead of hardcoding these pin numbers.

| Port | Teensy Pin | Function              | Notes                          |
|------|------------|------------------------|---------------------------------|
| AUX1 | 28         | digital, PWM, UART RX  | Serial7 RX                     |
| AUX1 | 29         | digital, PWM, UART TX  | Serial7 TX                     |
| AUX2 | 24         | digital, PWM, UART TX, analog (A10) | Serial6 TX; currently driving the shift light neopixel strip |
| AUX2 | 25         | digital, PWM, UART RX, analog (A11) | Serial6 RX                     |

