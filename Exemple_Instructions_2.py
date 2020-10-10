#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Sep 12 16:42:41 2020

@author: cybermimile

Turn on PB0 if PB1 is HIGH, turn off otherwise (for a given amount of time)
"""

from atmega8 import PB0, PB1, ATMega8, INPUT, OUTPUT, LOW
import time
timeout = 30 #The program runs for 30 seconds

at = ATMega8('/dev/ttyUSB0')
at.pinMode(PB0,OUTPUT)
at.pinMode(PB1,INPUT)
at.digitalWrite(PB1,LOW)

instructions = []
instructions.append('IFPINB_1\r')
instructions.append('WRPINB_0_1\r')
instructions.append('WRPINB_0_0\r')
at.instructionsWrite(instructions)

start_time = time.time()
current_time = time.time()
at.instructionsLoop()
while ((current_time-start_time)<timeout):
    time.sleep(1)
    current_time = time.time()
    print(current_time-start_time)
print("Done")
at.instructionsBreak()