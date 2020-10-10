#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Sep 12 11:42:47 2020

@author: cybermimile

Control a small servomotor on PB1 using a PWM signal
"""


import atmega8
import time

MAXANGLE = 170 #Maximum angle of the servomotor in degrees
MAXPULSE = 5000 #Duration of the pulse in microseconds for this angle

class Servo:
    def __init__(self,atm8,pin,max_angle=180,max_pulse=2000):
        self.atmega8 = atm8
        self.atmega8.pinMode(pin,atmega8.OUTPUT)
        self.pin = pin
        self.max_angle = max_angle
        self.max_pulse = max_pulse
    def rotate(self,angle):
        """Rotate the servo at the desired angle"""
        pulse = 1000 + (angle*(self.max_pulse-1000))//self.max_angle
        print(pulse)
        self.atmega8.servo(self.pin,pulse)
        
        
        

at = ATMega8('/dev/ttyUSB0')
servo = Servo(at,atmega8.PB1,max_angle=MAXANGLE,max_pulse=MAXPULSE)
angle = 0
servo.rotate(0)
while(angle<MAXANGLE):
    servo.rotate(angle)
    angle +=20
    time.sleep(1)
while(angle>=0):
    servo.rotate(angle)
    angle -=20
    time.sleep(1)