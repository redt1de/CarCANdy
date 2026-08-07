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


# RJ45 Wiring (Gold pins facing you, clip on top)
        ___
-------|   |-------
|                 | 
| 8 7 6 5 4 3 2 1 |
| | | | | | | | | |
-------------------

1: CAN3_L
2: CAN3_H
3: GND
4: CAN2_L
5: NA
6: CAN2_H
7: CAN1_H
8: CAN1_L



make it hacken