# Arduinolcdi2c
 Creation of a brightness control system and two further analogue inputs:
 a) A temperature sensor https://docs.wokwi.com/parts/wokwi-ntc-temperature-sensor 
 b) A potentiometer https://docs.wokwi.com/parts/wokwi-slide-potentiometer 
 Shows on a lcd1602 i2c the value of the 3 analogue inputs.
 The measurements must be carried out BY AUTONOMOUS PROCESSES, activated by a TIMER, 
 at intervals of 10 seconds and each measurement must be the average of 64 consecutive measurements.
 ADCs must be managed via interrupts.
 Write operations on the display (WRITE OPERATION) must be performed using the basic commands (INSTRUCTION SET) defined in the datasheet.(1602) 
 [For example: https://www.waveshare.com/datasheet/LCD_en_PDF/LCD1602.pdf ) and "interrupt driven" (any form of "delay" is FORBIDDEN).
 The design must be carried out according to the paradigm:
• Fewer resources used for the machine
• Greater effort for design and development.
In particular:
a) HW-SW components must be independent "Automata" that communicate with global variables
b) Timings (delays etc.) must be governed by the clock's ISR
c) The design of the ISR must guarantee service times compatible with the real-time responses of the system.
d) I/O management must be direct, without using "Arduino" functions or external libraries.
