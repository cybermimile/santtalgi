#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Sep 12 15:05:55 2020

@author: cybermimile

Several examples for using the instructions recording feature
(uncomment the part you want to try)
"""

import atmega8
import time
at = atmega8.ATMega8('/dev/ttyUSB0')
pin = atmega8.PB0

at.pinMode(pin,atmega8.OUTPUT)
instructions = []
instructions.append('BLPINB_0_500_M\r')
instructions.append('MCWAIT_500_M\r')


## --- Blink the pin for a given number of times
#NBLINK = 12 #Number of times to blink
#at.instructionsWrite(instructions)
#at.instructionsExecute(NBLINK)

## --- Blink for 5 seconds
#start_time = time.time()
#current_time = time.time()
#timeout = 5
#at.instructionsWrite(instructions)
#at.instructionsLoop()
#while ((current_time-start_time)<timeout):
#    time.sleep(1)
#    current_time = time.time()
#    print(current_time-start_time)
#print("Done")
#at.instructionsBreak()

## --- Blink while PB1 high
#instructions.append('IFPINB_1\r')
#instructions.append('INGOTO_0\r')
#at.pinMode(atmega8.PB1,atmega8.INPUT)
#at.digitalWrite(atmega8.PB1,atmega8.INPUT)
#at.instructionsWrite(instructions)
#at.instructionsExecute()
