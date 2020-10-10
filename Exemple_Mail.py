#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Sep 26 14:08:40 2020

@author: cybermimile

Turn on a LED on PB0 if unread messages
I am using a DAUM mailbox, just change the imap server if you have a different mailbox

To avoid writing the username and password in clear in the sketch, I am using environment variables.
To set the environment variables on ubuntu, add the following lines to the file .profile of the home directory:
export DAUM_USER="yourusername"
export DAUM_PWD="yourpassword"
"""

import os
import imaplib
import atmega8

pin_led = atmega8.PB0

def check_emails(mailbox):
    mailbox.select("Inbox")
    (status, list_mails) = mailbox.search(None, '(UNSEEN)')
    return list_mails[0].split()

#Login to the mailbox
username = os.environ.get('DAUM_USER') # username stored in environment variable DAUM_USER
password = os.environ.get('DAUM_PWD') # password stored in environment variable DAUM_PWD
imapserver = 'imap.daum.net' #Change with your mailbox imap server address
daumbox = imaplib.IMAP4_SSL(imapserver)
daumbox.login(username,password)

#Initialize the atmega
at = atmega8.ATMega8('/dev/ttyUSB0')
at.pinMode(pin_led,atmega8.OUTPUT)

#Checking email and updating PB0 accordingly
email_list = check_emails(daumbox)
num_emails = len(email_list)
if num_emails>0:
    print('Unread messages: '+str(num_emails))
    at.digitalWrite(pin_led,atmega8.HIGH)
else:
    print('No unread messages')
    at.digitalWrite(pin_led,atmega8.LOW)


