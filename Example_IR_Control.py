#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Sep  6 17:41:19 2020

@author: cybermimile

Control various functions of Clementine using the IR remote
"""

from atmega8 import ATMega8, PD2, INPUT
import os

KeyWords = {
33161726: "clementine -l \"/media/cybermimile/DATA/Musique/Weezer/Weezer\" &", #Power on (insert the folder of your playlist)
33227006: "clementine --volume-up", #Vol+
33194366: "clementine -s", #Func/stop
33178046: "clementine -r", #Backward
33243326: "clementine -t", #Play/Pause
33210686: "clementine -f", #Forward
33169886: "", #Down
33235166: "clementine --volume-down", #Vol-
33202526: "", #Up
33186206: "clementine --play-track 0", #0
33251486: "", #Eq
33218846: "", #St/Rept
33165806: "clementine --play-track 1", #1
33231086: "clementine --play-track 2", #2
33198446: "clementine --play-track 3", #3
33182126: "clementine --play-track 4", #4
33247406: "clementine --play-track 5", #5
33214766: "clementine --play-track 6", #6
33173966: "clementine --play-track 7", #7
33239246: "clementine --play-track 8", #8
33206606: "clementine --play-track 9" #9
}

Exit_Word = 33251486 #Eq

def IR_handler(word,is_valid,pulses):
    print("Word received: "+str(word))
    if word == Exit_Word:
        return 1
    try:
        os.system(KeyWords[word])
    except:
        print("Unknown word")
    return 0 #don't break waiting loop

at = ATMega8('/dev/ttyUSB0')
at.pinMode(PD2,INPUT)
at.decodeIR(IR_handler)
