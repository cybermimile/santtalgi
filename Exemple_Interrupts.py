#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Sep  6 16:41:59 2020

@author: cybermimile

Execute a command (play a tune) when an interrupt is received (PD2 rising).
"""

from atmega8 import PD2, ATMega8
import os

command = "clementine -l \"/media/cybermimile/DATA/Musique/Weezer/Weezer/04 - Island In The Sun.mp3\"" 

def interrupt_handler():
    print("Interrupt received")
    print(os.system(command))
    return 1 #break waiting loop

at = ATMega8('/dev/ttyUSB0')
at.attachInterrupt(PD2,interrupt_handler,"Rising")