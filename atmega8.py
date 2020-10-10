# -*- coding: utf-8 -*-
"""
Created on Tue Aug 18 12:48:24 2020

@author: cybermimile
"""

import serial
import time


# datetime object containing current date and time

#------------ Constants
VERBOSE = 1 #Print the messages on the console -- Errors always printed

ATMega8_Errors = {
"1": "Invalid command (too short)",
"2": "Invalid command",
"3": "Invalid argument (wrong size)",
"4": "Invalid argument 0 (value)",
"5": "Invalid argument 1 (value)",
"6": "Invalid command (not recognized)",
"7": "Invalid pin (reserved)",
"8": "Invalid pin number (out of range)",
"9": "Invalid argument 3 (value S, M, U)",
"A": "Too many instructions in list",
"B": "Execution interrupted by user",
"C": "IFPIN need onefollow-up instructions"
}

DEF_WAITING = 1 # Default waiting time after sending a message (in seconds)
NINSTR = 10 # Maximal number of instructions that can be recorded

HIGH = 1
LOW = 0
OUTPUT = 1
INPUT = 0

SECOND = 'S'
MILLI = 'M'
MICRO = 'U'

PORTB = 'B'
PORTC = 'C'
PORTD = 'D'
DDRB = 'B'
DDRC = 'C'
DDRD = 'D'

#Dictionary frequency (kHz) - prescaler
PWM1_Freq = {
"12000" : '1',
"1500" : '2',
"187" : '3',
"47" : '4',
"12" : '5'
}

PWM2_Freq = {
"12000" : '1',
"1500" : '2',
"375" : '3',
"187" : '4',
"94" : '5',
"47" : '6',
"12" : '7'
}

#Modes of interrupts
INT_modes = { 
"LOW" : 'L',
"Toggle" : 'T',
"Falling" : 'F',
"Rising" : 'R'
}

#------------- Extra functions
def printv(msg):
    if(VERBOSE==1):
        print(msg)

def getpower(x):
    """Return (value, power) such that x = value * 10**(power) and value < 10**4"""
    power = 0
    while (x//10**(power)>(10**4-1)):
        power +=1
    return x//10**(power) ,  power

def test_function():
    """A test function for attaching the interrupts
    Should return 0 if we continue waiting, non-zero break the waiting loop"""
    print("An interrupt has been received")
    return 1 #Non-zero return to break the waiting loop

def test_function_IR(word,is_valid,pulses):
    printv("The word is: "+str(word))
    return 0 #Non-zero return to break the waiting loop

    
#------------- Class definition
#----- Pins
class ATM8_pin:
    """This class represents the pins on the ATMEGA8, they have a port and a pin number"""
    def __init__(self,port,number):
        self.port = port
        self.number = number
    def __eq__(self,other):
        if(self.port==other.port) and (self.number==other.number):
            return 1
        else:
            return 0

PB0 = ATM8_pin('B',0)
PB1 = ATM8_pin('B',1)
PB2 = ATM8_pin('B',2)
PB3 = ATM8_pin('B',3)
PB4 = ATM8_pin('B',4)
PB5 = ATM8_pin('B',5)
PC0 = ATM8_pin('C',0)
PC1 = ATM8_pin('C',1)
PC2 = ATM8_pin('C',2)
PC3 = ATM8_pin('C',3)
PC4 = ATM8_pin('C',4)
PC5 = ATM8_pin('C',5)
PD2 = ATM8_pin('D',2)
PD3 = ATM8_pin('D',3)
PD4 = ATM8_pin('D',4)
PD5 = ATM8_pin('D',5)
PD6 = ATM8_pin('D',6)
PD7 = ATM8_pin('D',7)


#----- ATMega8
class ATMega8:
    """This class represents the ATMega8 chip and provide the methods for communication"""
    def __init__(self,serial_port='/dev/ttyUSB0',baud_rate=57600):
        self.port = serial.Serial(serial_port, baud_rate, timeout=2)
    def SendMsg(self,msg,waiting=DEF_WAITING):
        msg = bytes(msg,'utf-8')
        self.port.write(msg)
#        time.sleep(waiting)
        line = self.port.readline()
        if (line==b"OK\n"):
            return 0
        else:
            line = line.decode("utf-8")
            if len(line)!=4:
                print("Error: Unexpected return from ATMega: "+line)
                return -1
            else:
                print("Error: "+ATMega8_Errors[line[2]])
                return line[2]
    def digitalWrite(self,pin,value):
        msg = 'WRPIN' + pin.port + '_' + str(pin.number) + '_'
        if value==HIGH:
            msg+='1\r'
        elif value==LOW:
            msg+='0\r'
        else:
            print('Error: non standard value (HIGH/LOW)')
            return
        return self.SendMsg(msg)
    def digitalBlink(self,pin,duration=500,unit='M'):
        if (duration<0) or (duration>9999):
            print('Error: invalid duration argument (0--9999)')
            return
        msg = 'BLPIN' + pin.port + '_' + str(pin.number) + '_' + str(duration) +'_' + unit + '\r'
#        msg = bytes(msg,'utf-8')
        waiting = DEF_WAITING
        if (unit == 'S') and (duration > DEF_WAITING):
            waiting = duration
        return self.SendMsg(msg,waiting)
    def pinMode(self,pin,value):
        msg = 'DRPIN' + pin.port + '_' + str(pin.number) + '_'
        if value==OUTPUT:
            msg+='1\r'
        elif value==INPUT:
            msg+='0\r'
        else:
            print('Error: non standard value (INPUT/OUTPUT)')
            return
#        msg = bytes(msg,'utf-8')
        return self.SendMsg(msg)
    def digitalRead(self,pin,waiting=DEF_WAITING):
        """ Return 0 for LOW, 1 for HIGH, -1 for ERROR"""
        msg = 'RDPIN' + pin.port + '_' + str(pin.number) + '\r'
        msg = bytes(msg,'utf-8')
        self.port.write(msg)
        line = self.port.readlines(waiting)
        result = line[len(line)-1]
        result = result.decode('utf-8')
        if (result[:2]=='RD') and (len(result)==5):
            if(result[3]=='0'):
                printv('LOW')
                return 0
            elif(result[3]=='1'):
                printv('HIGH')
                return 1
            else:
                print("Error: Unexpected return from ATMega: "+result)
                return -1
        elif (result[:2]=='ER') and (len(result)==4):
                print("Error: "+ATMega8_Errors[result[2]])
                return -1
        else:
            print("Error: Unexpected return from ATMega: "+result)
            return -1
    def analogRead(self,pin,waiting=DEF_WAITING):
        """ Return value 0--1023 if successful, -1 for ERROR"""
        if pin.port != 'C':
            print("Error: ADC only on PC pins!!!")
            return -1
        msg = 'RAPINC_' + str(pin.number) + '\r'
        msg = bytes(msg,'utf-8')
        self.port.write(msg)
        line = self.port.readlines(waiting)
        result = line[len(line)-1]
        result = result.decode('utf-8')
        if (result[:2]=='RD'):
            self.port.readlines(waiting) # To get read of 'OK'
            if result=="RD:\n":
                printv(0)
                return 0
            else:
                value = int(result[3:])
                printv(value)
                return value
        elif (result[:2]=='ER') and (len(result)==4):
                print("Error: "+ATMega8_Errors[result[2]])
                return -1
        else:
            print("Error: Unexpected return from ATMega: "+result)
            return -1
    def reset(self,waiting=DEF_WAITING):
        """ Return 0 if successfull, -1 Otherwise"""
        self.port.write(b'SRESET\r')
        time.sleep(waiting)
        l0 = self.port.readlines(DEF_WAITING)
        l1 = self.port.readlines(DEF_WAITING)
        l2 = self.port.readlines(DEF_WAITING)
        printv(l0[0].decode('utf-8'))
        printv(l1[0].decode('utf-8'))
        if (l2[0]==b"OK\n"):
            return 0
        else:
            return -1
    def portWrite(self,port,value):
        if (value>255) or (value<0):
            print("Errror: Invalid register value (0--255)")
            return
        msg = 'WRPRT' + port + '_' + str(value) +'\r'
#        msg = bytes(msg,'utf-8')
        return self.SendMsg(msg)
    def ddrWrite(self,ddr,value):
        if (value>255) or (value<0):
            print("Errror: Invalid register value (0--255)")
            return
        msg = 'WRDDR' + ddr + '_' + str(value) +'\r'
#        msg = bytes(msg,'utf-8')
        return self.SendMsg(msg)
    def analogueWrite1(self,pin,duty,freq='12'):
        """Set a PWM signal of duty 0--1023 on either PB1 or PB2"""
        if(pin!=PB1) and (pin!=PB2):
            print("Errror: Invalid pin (PB1-PB2)")
            return
        if(duty>1023) or (duty<0):
            print("Errror: Invalid duty (0--1023)")
            return
        msg = 'FPWMB' + str(pin.number) + '_' + PWM1_Freq[freq] + '_' + str(duty) + '\r'
#        msg = bytes(msg,'utf-8') 
        return self.SendMsg(msg)
    def analogueWrite2(self,pin,duty,freq='12'):
        """Set a PWM signal of duty 0--255 on PB3"""
        if(pin!=PB3):
            print("Errror: Invalid pin (PB3)")
            return
        if(duty>255) or (duty<0):
            print("Errror: Invalid duty (0--1023)")
            return
        msg = 'FPWMB' + str(pin.number) + '_' + PWM1_Freq[freq] + '_' + str(duty) + '\r'
#        msg = bytes(msg,'utf-8') 
        return self.SendMsg(msg)
    def analogueStop(self,pin):
        """Stop the PWM signal on pin"""
        if(pin!=PB1) and (pin!=PB2) and (pin!=PB3):
            print("Errror: Invalid pin (PB1-PB2-PB3)")
            return
        msg = 'SPWMB' + str(pin.number) + '\r'
#        msg = bytes(msg,'utf-8')
        return self.SendMsg(msg)
    def tone(self,pin,freq):
        if(pin!=PB1) and (pin!=PB2) and (pin!=PB3):
            print("Errror: Invalid pin (PB1-PB2-PB3)")
            return
        value, power = getpower(x)
        msg = 'WSQRB' + str(pin.number) + '_' + str(value) + '_' + str(power) + '\r'
#        msg = bytes(msg,'utf-8')
        return self.SendMsg(msg)
    def noTone(self,pin):
        """Stop the square signal on pin"""
        if(pin!=PB1) and (pin!=PB2) and (pin!=PB3):
            print("Errror: Invalid pin (PB1-PB2-PB3)")
            return
        msg = 'SSQRB' + str(pin.number) + '\r'
#        msg = bytes(msg,'utf-8')
        return self.SendMsg(msg)
    def servo(self,pin,pulse):
        """Start the control of a servomotor on pin, with pulse in microseconds"""
        if(pulse<1000 or pulse>9999):
            print("Error: pulse out of range (1000-9999)")
            return
        if(pin==PB1):
            msg = 'SERVB1_'
        elif(pin==PB2):
            msg = 'SERVB1_'
        else:
            print("Errror: Invalid pin (PB1-PB2-PB3)")
            return
        msg += str((2*(pulse-1000))//3)+'\r'
        return self.SendMsg(msg)
    def attachInterrupt(self, pin, function, mode="LOW",timeout=-1):
        """ Attach interrupt on pin PD2 (INT0) or PD3 (INT1)
            Then read serial port for timeout seconds
            if timeout<0 there is no timeout (default)"""
        if (pin==PD2):
            msg = 'WRINT0_' + INT_modes[mode] + '\r'
            at_return = b"INT0\n"
            msg_end = 'STINT0\r'
        elif (pin==PD3):
            msg = 'WRINT1_'+ INT_modes[mode] + '\r'
            at_return = b"INT1\n"
            msg_end = 'STINT1\r'
        else:
            print("Errror: Invalid pin (PD2-PD3)")
            return
#        msg = bytes(msg,'utf-8')
#        msg_end = bytes(msg_end,'utf_8')
        result = self.SendMsg(msg)
        if result != 0: #Attaching interrupt unsuccessfull
            return result
        #Start the reading process
        start_time = time.time()
        current_time = time.time()
        getting_out = 0
        while getting_out==0 and ((current_time-start_time)<timeout or (timeout<0)):
            current_time = time.time()
            line = self.port.readline()
            if (line==at_return):
                getting_out = function()
        while(line==at_return): # To get read of extra INT after exiting
            line =self.port.readline()
        return self.SendMsg(msg_end)
    def decodeIR(self, function, timeout=-1):
        """Call a function when an infrared signal is detected on PD2
        The function should have 3 arguments : word (int), is_valid (bool), pulses (str)"""
#        RETURN: 'IR Decode:' + table of recorded pulses separated with ':' (alternating HIGH/LOW) + '\nSignal valid\nWord' + word (int32)
        start_time = time.time()
        current_time = time.time()
        getting_out = 0
        msg = 'IRDCD0\r'
        if(self.SendMsg(msg)!=0):
            printv("Decoding IR failed")
            return -1
        while getting_out==0 and ((current_time-start_time)<timeout or (timeout<0)):
            current_time = time.time()
            line = self.port.readline().decode('utf-8')
            if(line[:10]=='IR Decode:'):
                printv("Signal Detected")
                pulses = line[11:-1]
                if (self.port.readline().decode('utf-8')=='Signal valid\n'):
                    is_valid = 1
                else:
                    is_valid = 0
                word = self.port.readline().decode('utf-8')
                try:
                    word = int(word[6:])
                    printv(pulses)
                    printv(is_valid)
                    printv(word)
                    getting_out = function(word,is_valid,pulses)
                except:
                    print("Error converting result:")                
        msg = 'STDCD0\r'
        return self.SendMsg(msg)
    def instructionsWrite(self,list_instr):
        """Record a list of instruction to be executed later"""
        if(len(list_instr)>NINSTR):
            print("Error: too many instructions (MAX="+str(NINSTR)+")")
            return
        self.port.write(b'WINSTR\r')
        printv(self.port.readline())
        for instr in list_instr:
            self.port.write(bytes(instr,'utf-8'))
            printv(self.port.readline())
        self.port.write(b'END\r')
        line = self.port.readline().decode('utf-8')
        if(line[3:]=="OK\n" or line[2:]=="OK\n"):
            printv("Success")
            return 0
        else:
            print("Error: failed to record instructions")
            return 1
    def instructionsExecute(self,loop=1):
        """Execute the instructions recorded a number of times given by the variable loop"""
        if(loop>9999):
            print("Error: maximum number of loops if 9999")
            return
        self.port.write(bytes('RINSTR_'+str(loop)+'\r','utf-8'))
#        self.port.write(b'EINSTR\r')
        printv(self.port.readline())
    def instructionsLoop(self):
        """Start executing the instructions recorded in the mode UINSTR (break when the USART receives 'q')"""
        self.port.write(b'UINSTR\r')
    def instructionsBreak(self):
        """Stop the execution of the instructions recorded by sending 'q' to the USART"""
        self.port.write(b'q')
        line = self.port.readline().decode('utf-8')
        if (line=="ERB\n"):
            return 0
        else:
            if len(line)!=4:
                print("Error: Unexpected return from ATMega: "+line)
                return -1
            else:
                print("Error: "+ATMega8_Errors[line[2]])
                return line[2]

if __name__ == '__main__':
    print("Running main")
    at = ATMega8('/dev/ttyUSB0')
    at.pinMode(PB0,OUTPUT)
    at.digitalWrite(PB0,LOW)
    at.digitalWrite(PB0,HIGH)
    at.digitalWrite(PB0,LOW)
    at.digitalWrite(PB0,HIGH)
    at.digitalWrite(PB0,LOW)
#    at.digitalBlink(PB0)
#    at.pinMode(PC0,INPUT)
#    print((at.analogRead(PC0)*5)/1023)
#    at.digitalWrite(PB0,HIGH)
#    instructions = []
#    instructions.append('BLPINB_0_500_M\r')
#    instructions.append('MCWAIT_500_M\r')
#    at.instructionsWrite(instructions)
#    at.instructionsExecute(6)
#    at.decodeIR(test_function_IR,timeout=15)
    
#x = 144
#print(getpower(x))
#at.tone(PB3,x)
#at.analogueWrite2(PB3,50,freq='1500')
#at.analogueStop(PB2)
#at.ddrWrite(DDRB,5)
#at.portWrite(PORTB,3)
#print(at.reset())
#at.digitalWrite(PB0,HIGH)
#at.pinMode(PC5,INPUT)
#print(at.analogRead(PC5))
#at.pinMode(PB0,INPUT)
#at.digitalRead(PB0)
#at.digitalBlink(PB0,500,MILLI)
#result = at.SendMsg(b'BLPINB_0_2_S\r')
#at.SendMsg(b'BLPINB_0_2_S\r')
