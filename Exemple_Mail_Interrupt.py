#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sun Sep  6 16:41:59 2020

@author: cybermimile

Send an email when an interrupt is received (PD2 Rising)

I am using a DAUM mailbox, just change the smtp server if you have a different mailbox

To avoid writing the username and password in clear in the sketch, I am using environment variables.
To set the environment variables on ubuntu, add the following lines to the file .profile of the home directory:
export DAUM_USER="yourusername"
export DAUM_PWD="yourpassword"
"""

import os
import atmega8
import smtplib
from email.message import EmailMessage

#Login to the mailbox
username = os.environ.get('DAUM_USER') # username stored in environment variable DAUM_USER
password = os.environ.get('DAUM_PWD') # password stored in environment variable DAUM_PWD
smtpserver = 'smtp.daum.net' #Or the address of your mailbox smtp server
sender = username+'@daum.net' #The sender email address
receiver = username+'@daum.net' #The receiver email address (yes, I am sending it to myself :)

def interrupt_handler():
    print("Interrupt received... sending email")
    # Create the container email message.
    msg = EmailMessage()
    msg['Subject'] = 'Interrupt on atmega'
    msg['From'] = 'cybermimile@daum.net'
    msg['To'] = 'cybermimile@daum.net'
    msg.set_content('The atmega has received an interrupt on PD2. All interrupts will be turned off.')
    with smtplib.SMTP_SSL(smtpserver,465) as smtp:
        smtp.login(username,password)
        smtp.send_message(msg)
    return 1 #break waiting loop

at = atmega8.ATMega8('/dev/ttyUSB0')
at.attachInterrupt(atmega8.PD2,interrupt_handler,"Rising")