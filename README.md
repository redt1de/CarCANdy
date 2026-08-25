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



