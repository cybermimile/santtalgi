#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Sep  6 14:39:53 2020

@author: cybermimile

Control the sound of ubuntu using an external pot (on PC5) and analogueRead function.
Need to install alsaaudio module: sudo apt-get install python3-alsaaudio
"""



import alsaaudio
import atmega8
import time

PLED = atmega8.PC5

at = atmega8.ATMega8('/dev/ttyUSB0')
mixer = alsaaudio.Mixer()
at.pinMode(PLED,atmega8.INPUT)
at.digitalWrite(PLED,atmega8.LOW)

while 1:    
    time.sleep(.5)
    volume = (at.analogRead(PLED)*100)//1023
    if (volume>=0) and (volume<=100):
        mixer.setvolume(volume)
#print(mixer.getvolume())
