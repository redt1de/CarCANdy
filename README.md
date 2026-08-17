# CarCANdy

# TODO:
- (V2) re-wire the RJ45 connector so pairs or together and in order
- add "live" capture to cli that will read from serial


# SN65HVD233DR Mode select pin (Rs): 
    high speed mode:    JP 2->1 and solder RS
    slope control mode: JP 2->1 and open RS
    low power mode:     JP 2->3


# Aux Ports
    include/AuxPorts.h
    AUX1: digital, UART,PWM
        28: RX,D28
        29: TX,D28

    AUX2: digital,analog,UART,PWM
        24: TX,D24,A10
        25: RX,D25,A11


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

1: CAN3_L - white/brown
2: CAN3_H - brown
3: GND - white/orange
4: CAN2_L - white/green
5: NA
6: CAN2_H - green
7: CAN1_H - blue
8: CAN1_L - white/blue



