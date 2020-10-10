#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Sep 27 19:23:01 2020

@author: cybermimile

Record a series of measures using an LM35DZ temperature sensor on PC0 and show the plot
"""

import atmega8
from time import sleep
import matplotlib.pyplot as plt

pin_sensor = atmega8.PC0
N_measures = 50
Delay = 0.2

at = atmega8.ATMega8('/dev/ttyUSB0')
at.pinMode(pin_sensor,atmega8.INPUT)
at.digitalWrite(pin_sensor,atmega8.LOW)

n=0
X = []
Y = []
while n<N_measures:
    value = at.analogRead(pin_sensor)
    temperature = (value * 500)/1023
    X.append(n*Delay)
    Y.append(temperature)
    sleep(Delay)
    n+=1
plt.figure()
plt.plot(X,Y)
#plt.show(block=False)

