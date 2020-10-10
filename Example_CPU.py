#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Sep 26 16:31:12 2020

@author: cybermimile

Turn on LEDs on PC0~PC5 according to CPU usage
0-10: no led
10-25: +PC0
25-40: +PC1
40-55: +PC2
55-70: +PC3
70-85: +PC4
85-100: +PC5
"""

import atmega8
import psutil

led_pins = [atmega8.PC0, atmega8.PC1, atmega8.PC2, atmega8.PC3, atmega8.PC4, atmega8.PC5]

cpu = psutil.cpu_percent()
at = atmega8.ATMega8('/dev/ttyUSB0')
at.ddrWrite(atmega8.DDRC,255)

print('CPU usage: '+str(cpu))
i = 0
while i<6:
#    print(i*15+10)
    if cpu>i*15+10:
        at.digitalWrite(led_pins[i],atmega8.HIGH)
    else:
        at.digitalWrite(led_pins[i],atmega8.LOW)
    i+=1
