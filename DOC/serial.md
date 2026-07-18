 USE  | UART	| RX | TX | Notes
------------------------------------------------
CAN2  | Serial1	| 0	 | 1  | Also CAN2 pins (FlexCAN)
ESP32 | Serial2	| 7	 | 8  | Excellent general-purpose choice
      | Serial3	| 15 | 14 | CTS available on pin 19 
      | Serial4	| 18 | 17 | Shares pins with I2C if needed
CAN1  | Serial5	| 22 | 20 | Alternate TX on 47, RX on 46
      | Serial6	| 25 | 24 | Shares pins with I2C2
AUX1  | Serial7	| 28 | 29 | Good general-purpose UART
      | Serial8	| 34 | 35 | CTS on pin 50