#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Sep 26 14:08:40 2020

@author: cybermimile

Display unread emails on a small LCD display 2x16 in 4-bits mode

I am using a DAUM mailbox, just change the imap server if you have a different mailbox

To avoid writing the username and password in clear in the sketch, I am using environment variables.
To set the environment variables on ubuntu, add the following lines to the file .profile of the home directory:
export DAUM_USER="yourusername"
export DAUM_PWD="yourpassword"
"""

import os
import imaplib
import email
import atmega8

username = os.environ.get('DAUM_USER') # username stored in environment variable DAUM_USER
password = os.environ.get('DAUM_PWD') # password stored in environment variable DAUM_PWD
imapserver = 'imap.daum.net' # the address of the imap server of your mailbox

def nibble(char):
    if char>255:
        print("Error: invalid command/character")
        return 0,0
    lower = char%16
    higher = (char-lower)//16
    return lower, higher

class LCD:
    """This class is used to control a small LCD display 2x16 in 4-bits mode
        Use the first 4 bits of port_DATA for the DATA pins"""
    def __init__(self,atmega,port_DATA,pin_RS,pin_RW,pin_EN):
        self.atmega8 = atmega
        self.port_DATA = port_DATA
        self.pin_RS = pin_RS
        self.pin_RW = pin_RW
        self.pin_EN = pin_EN
        self.atmega8.ddrWrite(port_DATA,255)
        self.atmega8.pinMode(pin_RS,atmega8.OUTPUT)
        self.atmega8.pinMode(pin_RW,atmega8.OUTPUT)
        self.atmega8.pinMode(pin_EN,atmega8.OUTPUT)
        self.initialize()        
        self.clear_screen()
    def initialize(self):
        self.send_cmd(0x02) #initialize in 4-bits mode
        self.send_cmd(0x28) #initialize in 5x7 dots
        self.send_cmd(0x0C) #make cursor invisible
#        self.send_cmd(0x06) 
#        self.send_cmd(0x83)
    def clear_screen(self):
        self.send_cmd(0x01) #clear screen
        self.send_cmd(0x02) #move cursor home
    def send_char(self,char):
        lchar, hchar = nibble(char)
        self.atmega8.digitalWrite(self.pin_RS,atmega8.HIGH)
        self.atmega8.portWrite(self.port_DATA,hchar)
        self.atmega8.digitalBlink(self.pin_EN,10,atmega8.MICRO)
        self.atmega8.portWrite(self.port_DATA,lchar)
        self.atmega8.digitalBlink(self.pin_EN,10,atmega8.MICRO)
    def send_cmd(self,cmd):
        lcmd, hcmd = nibble(cmd)
        self.atmega8.digitalWrite(self.pin_RS,atmega8.LOW)
        self.atmega8.portWrite(self.port_DATA,hcmd)
        self.atmega8.digitalBlink(self.pin_EN,10,atmega8.MICRO)
        self.atmega8.portWrite(self.port_DATA,lcmd)
        self.atmega8.digitalBlink(self.pin_EN,10,atmega8.MICRO)
    def write_line1(self,string): #Write the string on the first line
        string = string[:16] # Keep only the first 16 characters
        self.send_cmd(0x02) #move cursor home
        for letter in string:
            self.send_char(ord(letter))
    def write_line2(self,string): #Write the string on the first line
        string = string[:16] # Keep only the first 16 characters
        self.send_cmd(0xC0) #move cursor to the beginning of the second line
        for letter in string:
            self.send_char(ord(letter))        

def check_emails(mailbox): #Return the list of unread emails
    mailbox.select("Inbox")
    (status, list_mails) = mailbox.search(None, '(UNSEEN)')
    return list_mails[0].split()
 
def get_email_info(mailbox,mail): #Get the information for a specific email
#    status, email_data = mailbox.uid('fetch',mail,'(RFC822)')
    status, email_data = mailbox.fetch(mail,'(RFC822)')
    raw_email = email_data[0][1].decode("utf-8")
    msg = email.message_from_string(raw_email)
    Subject = msg['subject']
    From = msg['from']
    if '<' in From:
        pos1 = From.index('<')
        pos2 = From.index('>')
        Address = From[pos1+1:pos2]
        From = From[:pos1-1]
    else:
        Address = From
    return Subject, From, Address

at = atmega8.ATMega8('/dev/ttyUSB0')
lcd = LCD(at,atmega8.PORTC,atmega8.PB0,atmega8.PB1,atmega8.PB2)
daumbox = imaplib.IMAP4_SSL(imapserver)
daumbox.login(username,password)
unread_mails = check_emails(daumbox)
num_unread = len(unread_mails)
lcd.write_line1('Unread:'+str(num_unread))
if num_unread>0:
    print('You have '+str(num_unread)+' unread emails')
    last_mail = unread_mails[-1]
    last_subject, last_from, last_address = get_email_info(daumbox,last_mail)
    print('The last email is from '+last_from+' with address '+last_address)
    print('It is about '+last_subject)
    lcd.write_line2(last_address)
    