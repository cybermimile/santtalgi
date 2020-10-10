#define F_CPU 12000000UL  // 12 MHz
#define F 120 //frequency in 0.1MHz

//Upload: sudo avrdude -v -pm8 -c usbtiny -U flas:w:Driver_ATMEGA8.hex

/* Error codes
1: Invalid command (too short)
2: Invalid command
3: Invalid argument (wrong size)
4: Invalid argument 0 (value)
5: Invalid argument 1 (value)
6: Invalid command (not recognized)
7: Invalid pin (reserved)
8: Invalid pin number (out of range)
9: Invalid argument 3 (value S, M, U)
A: Too many instructions in list
B: Execution interrupted by user
C: IFPIN need onefollow-up instructions
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#define USART_BAUDRATE 57600 // Define baud rate
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define MAXSTR 50 // Maximal length for strings to transmit
#define SIZE_CMD 6 //Size command key words
#define SIZE_ARG 4 //Size argument
#define SIZE_COMMAND 20 //Total command size ~6+3*4+4
#define N_ARG 3 //Number of arguments
#define N_INSTR 10 //Maximal number of instructions
//IR decoding constants
#define MAXPULSE 20000 //Number of pulses for IR
#define RES 5
#define EBAR 100 // Check up to error bar
#define NBPULSE 50 // number of pulses in the tab

volatile unsigned char value;
volatile uint8_t is_IR_on;

unsigned char *msg0 = "Driver for ATMEGA_8\n";
unsigned char *msg1 = "Reset complete...waiting for instructions\n";
volatile char COMMAND[SIZE_COMMAND];
volatile char CMD[SIZE_CMD+1];
volatile char ARG[N_ARG][SIZE_ARG];
unsigned char u8TempData;
volatile char INSTR_CMD[N_INSTR][SIZE_COMMAND];
volatile uint8_t INSTR_end;
volatile uint8_t INSTR_current;

//IR decoding variables
volatile uint16_t pulses[NBPULSE][2]; // pair is high and low pulse
volatile uint16_t currentpulse = 0; // index for pulses we're storing
volatile uint16_t lengthpulse =0 ; // length of the pulses

uint16_t PRES1[5]={1,8,64,256,1024};
uint16_t PRES2[7]={2,8,32,64,128,256,1024};
//--------------------- Functions declaration
char reset(void);
void USART_SendByte(uint8_t u8Data);
char CMD_Decode(void);
//---------------------- Basic functions

unsigned long int power_ten32(uint8_t i){
    if(i==0) return 1;
    else return 10*power_ten32(i-1);
}

uint16_t power_ten(uint8_t i){
    if(i==0) return 1;
    else return 10*power_ten(i-1);
}

void delay_ms(uint16_t imax){ //Wait for imax milliseconds
    uint16_t i=0;
    for(i=0;i<imax;i++) _delay_loop_2(25*F);
}

void delay_s(uint16_t imax){
    uint16_t i=0;
    for(i=0;i<imax;i++) delay_ms(1000);
}

void delay_us(uint16_t imax){
    uint16_t i;
    i = imax;
    while (i>7680/F){//FCPUdelay up to 64us = 256*3...
        _delay_loop_1(255);
        i -= 7680/F;
    }
    _delay_loop_1((i*10)/F);
}


void test_blink(char n) //Test code: blink LED on PA
{
    PORTB|=(1<<n);
    delay_ms(500);
    PORTB&=~(1<<n);
    delay_ms(500);
}

uint8_t arg_len(uint8_t arg_num){//Determine the length of the argument
    uint8_t length;
    length = 0;
    while((length<SIZE_ARG)&&(ARG[arg_num][length]!='\0')) {
        length++;
        //USART_SendInt(length);
        //USART_SendByte(':');
        }
//    USART_SendByte(':');
//    USART_SendByte(':');
//    USART_SendInt(length);
//    USART_SendByte('\n');
    return length;
}

uint16_t arg_int(uint8_t arg_num) {
    uint8_t length;
    length = arg_len(arg_num);
    //USART_SendInt(arg_num);
    //USART_SendByte(':');
    //USART_SendInt(length);
    //USART_SendByte(':');
    uint16_t result;
    uint8_t i;
    result = 0;
    i = 0;
    while(i<length) {// Does not happen if length = 0
        if (ARG[arg_num][i]<58&&ARG[arg_num][i]>47) result += (ARG[arg_num][i]-48)*power_ten(length-i-1); //check if this is a digit '0'=48 ... '9'=57 in ASCII
        i++;}
    //USART_SendInt(result);
    //USART_SendByte('\n');
    return result;
}

uint8_t equ(uint16_t a, uint16_t b) {
  if(abs(a-b)<EBAR) return 1;
  else return 0;
}

// ---------- Interrupts

ISR(INT0_vect) {
    cli();
    if(is_IR_on!=1) USART_SendMsg("INT0\n");
    else { //Decode IR signal
        currentpulse=0;
        while(1) {
            lengthpulse=0; // start out with no pulse length
            while(PIND&(1<<PIN2)) {// pin is still HIGH
                lengthpulse++; // count off another few microseconds
                _delay_loop_2(RES); // Delay of 20us
                if((lengthpulse>=MAXPULSE)&&(currentpulse!=0)) {// If the pulse is too long, we 'timed out' - either nothing was received or the code is finished, so print what we've grabbed so far, and then reset
                    printpulses();
                    return;}
                }
        // we didn't time out so lets stash the reading
        pulses[currentpulse][0] = (lengthpulse*2)/3; //Add 2/3 factor for 12MHz instead of 8MHz
        lengthpulse=0;
        // same as above
        while(!(PIND&(1<<PIN2))) {// pin is still LOW
            lengthpulse++;
            _delay_loop_2(RES); // Delay of 20us
            if((lengthpulse>=MAXPULSE)&&(currentpulse!=0)) {
                printpulses();
                return;}
        }
        pulses[currentpulse][1] = (lengthpulse*2)/3; //Add 2/3 factor for 12MHz instead of 8MHz
        // we read one high-low pulse successfully, continue!
        currentpulse++;
        } // fin while1
    }
}

ISR(INT1_vect) {
    cli();
    USART_SendMsg("INT1\n");
}


// ---------- IR decode


void shift_pulses(uint8_t i){
    uint8_t j;
  for(j=0; j<currentpulse-i;j++) {
    pulses[j][0]=pulses[i+j][0];
    pulses[j][1]=pulses[i+j][1];
  }
}

uint8_t is_start_signal(){
    uint16_t i;
    for(i=0; i<currentpulse;i++){// Look for the start signal at the position i
    if((equ(pulses[i][1],1720)+equ(pulses[i+1][0],850)+equ(pulses[i+1][1],105))==3) { //Code for 8MHz
    //if((equ(pulses[i][1],2700)+equ(pulses[i+1][0],1330)+equ(pulses[i+1][1],170))==3) {
    // Signal trouve
    if(i!=0) {shift_pulses(i); currentpulse-=i;} //Erase the data before start signal, and shift the table
    return 1;}
  }
  return 0;
}

uint8_t end_signal(){//Look for the ending signal
  uint8_t i;
  uint16_t p1,p2;
  for(i=0;i<currentpulse-1;i++){
      p1= (pulses[i][0]/8); // Because tolerence EBAR is too small for those two
      p2= (pulses[i][1]/8); // We divide them by two before comparison
  if(equ(p1,981)&&equ(p2,215)&&equ(pulses[i+1][0],430)&&equ(pulses[i+1][1],105)) return i;
  //if(equ(p1,981)&&equ(p2,215)&&equ(pulses[i+1][0],430)&&equ(pulses[i+1][1],105)) return i;
  }
  return currentpulse; //if not found, return total size
}

unsigned long int build_word(uint8_t e){
  unsigned long int result=0;
  uint8_t i;
  for(i=2;i<e;i++){ //Do not forget to add the offset of start signal
  if(equ(pulses[i][1],105)) {//signal ON valid
  if(equ(pulses[i][0],320)) {result+=1; result = result << 1;}//bit 1 for H
  else if(equ(pulses[i][0],105)) result = result << 1; //bit a 0 for L
  // Sinon skip le signal car invalide
  }
  }
  return result;
}

void printpulses(void) {
    uint8_t i;
    USART_SendMsg("IR Decode:");
    if(currentpulse>10) {//Otherwise must be noise
    for(i=0; i<currentpulse;i++){// Print out the row data
        USART_SendInt(pulses[i][1]);
        USART_SendByte(':');
        USART_SendInt(pulses[i][0]);
        USART_SendByte(':');}
    USART_SendByte('\n');
    if(is_start_signal()==1) {// Signal valid
        USART_SendMsg("Signal valid\n");
        uint8_t es=end_signal();
        if(es!=currentpulse) {
            USART_SendMsg("Word:");
            USART_SendInt32(build_word(es));
            USART_SendByte('\n');
//    delay_ms(500); //In case signal has been repeated
    sei(); //Re-allow interrupt
}}}}


// ---------- Operations


void INSTR_Clear(uint8_t n){
    uint8_t i;
    for(i=0;i<SIZE_COMMAND;i++) INSTR_CMD[n][i]='\0';
}

char INSTR_List(void){
    uint8_t n;
    for(n=0;n<INSTR_end;n++) {
    USART_SendInt(n+1);
    USART_SendByte(':');
    USART_SendMsg(INSTR_CMD[n]);
    USART_SendByte('\n');
    }
    return '0';
}

char INSTR_Record(void){
    uint8_t n; //Instruction number
    n = 0;
    uint8_t position;
    while(n<N_INSTR){
        position = 0;
        INSTR_Clear(n);
        USART_SendByte('\n');
        USART_SendInt(n+1);
        USART_SendByte(':');
        do {
        if ((UCSRA & (1<<RXC))) {
            INSTR_CMD[n][position] = UDR;
            position++;
        }} while(INSTR_CMD[n][position-1]!=13&&position<SIZE_COMMAND); //13 is ASCII character RETURN / newline
        INSTR_CMD[n][position-1] = '\0'; //Termination character instead of RETURN
        //USART_SendMsg(INSTR_CMD[n]);
        if(strcmp(INSTR_CMD[n],"END")==0) {
            INSTR_end = n;
            return '0';}
        n++;}
    return 'A';
}

char INSTR_Execute(void){
    INSTR_current = 0;
    char c;
    c = '0';
    while(INSTR_current<INSTR_end&&c=='0') {
        strcpy(COMMAND,INSTR_CMD[INSTR_current]);
        //USART_SendMsg(COMMAND);
        c = CMD_Decode();
        INSTR_current++;
    }
    return c;
}




char INSTR_Repeat(void){
    uint8_t LOOP_n;
    uint8_t LOOP_end;
    LOOP_n = 0;
    LOOP_end = arg_int(0);
    char c;
    c = '0';
    while(LOOP_n<LOOP_end&&c=='0') {
        INSTR_current = 0;
        while(INSTR_current<INSTR_end&&c=='0') {
            strcpy(COMMAND,INSTR_CMD[INSTR_current]);
            //USART_SendMsg(COMMAND);
            c = CMD_Decode();
            INSTR_current++;}
        LOOP_n++;}
    return c;
}

char INSTR_Until(void){
    char c;
    c = '0';
    while(c=='0') {
        INSTR_current = 0;
        while(INSTR_current<INSTR_end&&c=='0') {
            strcpy(COMMAND,INSTR_CMD[INSTR_current]);
            //USART_SendMsg(COMMAND);
            c = CMD_Decode();
            INSTR_current++;
            if ((UCSRA & (1<<RXC))&&UDR=='q') c = 'B';}
        }
    return c;
}

char INSTR_Goto(void){
    uint8_t where;
    where = arg_int(0);
    if(where>INSTR_end) return '4';
    INSTR_current = where-1;
    return '0';
}

char INSTR_Ifpin(volatile uint8_t *port){
    if (INSTR_current+2>INSTR_end) return 'C';
    if(strlen(ARG[0])!=1) return '3';
    if(ARG[0][0]>55||ARG[0][0]<48) return '4'; //Need the argument to be a number between '0'=48 and '7'=55
    uint8_t mask;
    mask = 1 << (ARG[0][0]-48);
    uint8_t val;
    val = *port;
//    USART_SendInt(val);
//    USART_SendByte(':');
    if((val&mask)==0) INSTR_current++; //If pin is LOW, skip the next instruction
    return '0';
}


char INTIR_Set(void) {
    cbi(DDRD,PIN2);
    cbi(PORTD,PIN2);
    MCUCR &= ~((1<<ISC01)|(1<<ISC00));//Interrupt low level INT0
    GICR |= (1<<INT0);
    is_IR_on = 1;
    sei();
    return '0';
}

char INTIR_Stop(void){
    GICR &= ~(1<<INT0);
    is_IR_on = 0;
    return '0';
}

char INT0_Set(void) {
    if(arg_len(0)>1) return '3';
    DDRD |= (1<<PIN2);
    if(ARG[0][0]=='L') {
        PORTD |= (1<<PIN2); //Pullup to avoid bouncing
        MCUCR &= ~((1<<ISC01)|(1<<ISC00));}
    else if (ARG[0][0]=='T') {
        sbi(MCUCR,ISC00);
        cbi(MCUCR,ISC01);}
    else if (ARG[0][0]=='F') {
        sbi(PORTD,PIN2); //Pullup to avoid bouncing
        cbi(MCUCR,ISC00);
        sbi(MCUCR,ISC01);}
    else if (ARG[0][0]=='R') {
        cbi(PORTD,PIN2); //No pullup to avoid bouncing
        MCUCR|= (1<<ISC00)|(1<<ISC01);}
    else return '4';
    GICR |= (1<<INT0);
    is_IR_on = 0;
    sei();
    return '0';
}

char INT0_Stop(void) {
    GICR &= ~(1<<INT0);
    return '0';
}

char INT1_Set(void) {
    if(arg_len(0)>1) return '3';
    DDRD |= (1<<PIN3);
    if(ARG[0][0]=='L') {
        PORTD |= (1<<PIN3); //Pullup to avoid bouncing
        MCUCR &= ~((1<<ISC11)|(1<<ISC10));}
    else if (ARG[0][0]=='T') {
        sbi(MCUCR,ISC10);
        cbi(MCUCR,ISC11);}
    else if (ARG[0][0]=='F') {
        sbi(PORTD,PIN3); //Pullup to avoid bouncing
        cbi(MCUCR,ISC10);
        sbi(MCUCR,ISC11);}
    else if (ARG[0][0]=='R') {
        cbi(PORTD,PIN3); //No pullup to avoid bouncing
        MCUCR|= (1<<ISC10)|(1<<ISC11);}
    else return '4';
    GICR |= (1<<INT1);
    sei();
    return '0';
}

char INT1_Stop(void) {
    GICR &= ~(1<<INT1);
    return '0';
}

char SQUARE_Set(uint8_t pin) {
    // Compute desired frequency f
    unsigned long int f;
    unsigned long int FCPU;
    FCPU = F * power_ten32(5);
    uint8_t power;
    power = arg_int(1);
    if(power>6) return '5';
    //USART_SendInt(arg_int(0));
    //USART_SendByte(':');
    f = arg_int(0)*power_ten32(power);
    //USART_SendInt32(f);
    //USART_SendByte('\n');
    if(f>FCPU) return '5';
    //USART_SendInt32(f);
    //USART_SendByte('\n');
    //Compute prescaler
    uint8_t N;
    unsigned long int Min;
    Min = FCPU/(2*f);
    uint8_t TCCR_reg;
    if(pin==3) {
        Min = Min/255;
        N=6;
        while(PRES2[N]>Min&&N>0) {N--;}
        if(N!=6&&PRES2[N]<Min) N++;
        TCCR_reg = N+1; // Fixes CS bits
        TCCR_reg += (1<<COM20); // Toggle pin
        TCCR_reg += (1<<WGM21); //CTC mode
        TCCR2 = TCCR_reg;
        OCR2 = FCPU/(2*PRES2[N]*f);
    }
    else {
        Min = Min/65535;
        N=4;
        while(PRES1[N]>Min&&N>0) {N--;}
        if(N!=4&&PRES1[N]<Min) N++;
        if(pin==1) {
            sbi(TCCR1A,COM1A0);
            cbi(TCCR1A,COM1A1);}
        else {
            sbi(TCCR1A,COM1B0);
            cbi(TCCR1A,COM1B1);}
        TCCR_reg = N+1;
        TCCR_reg += (1<<WGM12); //CTC mode
        TCCR1B = TCCR_reg;
        OCR1A = FCPU/(2*PRES1[N]*f);
        //USART_SendInt(OCR1A);
        //USART_SendByte(':');
        //USART_SendInt(N);
        }
    return '0';
}

char FPWM_Set(uint8_t pin) {
    uint8_t prescaler;
    uint16_t duty;
    prescaler = arg_int(0);
    duty = arg_int(1);
    if((pin!=3&&prescaler>5)||prescaler>7) return '4'; //Important because it will feed TCCR
    if(pin==3) {
        if(duty>255) return '5';
        DDRB |= (1<<PIN3);
        prescaler += (1 << WGM21) | (1 << WGM20) | (1 << COM21);//Add the bits for setting fast PWM mode
        TCCR2 = prescaler;
        OCR2 = duty;
    }
    else { //pin is 1 or 2
        if(duty>1023) return '5';
        DDRB |= (1 << (pin-1));
        if(pin==1) TCCR1A |= (1<<COM1A1) | (1<<WGM11) | (1<<WGM10);
        else TCCR1A |= (1<<COM1B1) | (1<<WGM11) | (1<<WGM10);
        prescaler += (1<<WGM12);
        TCCR1B = prescaler;
//        double dduty;
//        dduty = (duty/100.0)*1023;  //10-bits resolution
        if (pin==1) OCR1A = duty;
        else OCR1B = duty;}
    return '0';
}

char FPWM_Servo(uint8_t pin) {
    uint16_t range;
    range = arg_int(0);
    //if(range>5000) return '4';
    DDRB |= (1 << (pin-1));
    ICR1 = 29999;
    TCCR1B =   (1<<WGM13) | (1<<WGM12) | (1<<CS11);
    if(pin==1) {
        TCCR1A &= ~((1<<COM1A0) | (1<<WGM10));
        TCCR1A |= (1<<COM1A1) | (1<<WGM11);
        OCR1A = 1499+range;
        }
    else {
        TCCR1A &= ~((1<<COM1B0) | (1<<WGM10));
        TCCR1A |= (1<<COM1B1) | (1<<WGM11);
        OCR1B = 1499+range;
        }
    return '0';
}

char FPWM_Stop(uint8_t pin) {
    if(pin==3) {
        TCCR2 &= ~((1<<COM20)|(1<<COM21)); //Disconnect pin OC2
        OCR2 = 0;} // Stop counter
    else if(pin==2) {
        TCCR1A &= ~((1<<COM1B0)|(1<<COM1B1));
        OCR1B = 0;}
    else {
        TCCR1A &= ~((1<<COM1A0)|(1<<COM1A1));
        OCR1A = 0;}
    return '0';
}


char RD_ADC(void){
    uint8_t pin;
    pin = arg_int(0);
    if(pin>5) return '4';
    pin += (1<<REFS0); //5V reference voltage
    ADMUX = pin;
    ADCSRA = (1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2); //Enable F/128
    ADCSRA |= (1<<ADSC); //Start conversion
    while((ADCSRA &(1<<ADIF)) == 0);
    uint16_t result;
    result = ADC;
    USART_SendRD();
    USART_SendInt(result);
    USART_SendByte('\n');
    return '0';
}

char EEPROM_WR()
{
    uint16_t address;
    address = arg_int(0);
    if(address>511) return '4';
    uint8_t length;
    length = arg_len(1);
    uint8_t i;
    for(i=0;i<length;i++){
        while(EECR & (1<<EEWE)); /* Wait for completion of previous write */
        EEAR = address+i; /* Set up address and data registers */
        EEDR = ARG[1][i];
        EECR |= (1<<EEMWE); /* Write logical one to EEMWE */
        EECR |= (1<<EEWE); /* Start eeprom write by setting EEWE */
    }
    return '0';
}

char EEPROM_CLEAR(){
    uint16_t i;
    for(i=0;i<512;i++){
        while(EECR & (1<<EEWE)); /* Wait for completion of previous write */
        EEAR = i; /* Set up address and data registers */
        EEDR = 0;
        EECR |= (1<<EEMWE); /* Write logical one to EEMWE */
        EECR |= (1<<EEWE); /* Start eeprom write by setting EEWE */
    }
    return '0';
}

char EEPROM_RD()
{
    uint16_t i;
    USART_SendRD();
    for(i=0;i<512;i++){
        while(EECR & (1<<EEWE)) ; /* Wait for completion of previous write */
        EEAR = i; /* Set up address register */
        EECR |= (1<<EERE); /* Start eeprom read by writing EERE */
        USART_SendByte(EEDR); /* Return data from data register */
    }
    USART_SendByte('\n');
    return '0';
}

char CHK_REG(volatile uint8_t *port) {
    if(strlen(ARG[0])!=1) return '3';
    // Make sure the registered we are about to modified are not used for a different purpose
    if((port==&PORTB||port==&DDRB)&&(ARG[0][0]=='6'||ARG[0][0]=='7')) return '7'; // XTAL
    if((port==&PORTC||port==&DDRC)&&(ARG[0][0]=='6'||ARG[0][0]=='7')) return '7'; // RESET
    if((port==&PORTD||port==&DDRD)&&(ARG[0][0]=='0'||ARG[0][0]=='1')) return '7'; // USART RX & DX
    return '0';
}

char BLINK(volatile uint8_t *port) {
    char error;
    error = CHK_REG(port);
    if(error!='0') return error;
    uint16_t t;
    t = arg_int(1);
    uint8_t n;
    n = arg_int(0);
    if(n>7) return '8';
    sbi(*port,n);
    if(ARG[2][0]=='S') delay_s(t);
    else if(ARG[2][0]=='M') delay_ms(t);
    else if(ARG[2][0]=='U') delay_us(t);
    else return '9';
    cbi(*port,n);
    /*
    if(ARG[2][0]=='S') delay_s(t);
    else if(ARG[2][0]=='M') delay_ms(t);
    else if(ARG[2][0]=='U') delay_us(t);
    */
    return '0';
}

char WAIT(void) {
    uint16_t t;
    t = arg_int(0);
    if(ARG[1][0]=='U') delay_us(t);
    else if(ARG[1][0]=='M') delay_ms(t);
    else if(ARG[1][0]=='S') delay_s(t);
    else return '5';
    return '0';
}

char WR_REG_BYTE(volatile uint8_t *port) {
    uint16_t value;
    value = arg_int(0);
    if(value>255) return '4';
    *port = value;
    return '0';
}

char WR_REG(volatile uint8_t *port) {
    char error;
    uint8_t pin;
    error = CHK_REG(port);
    if(error!='0') return error;
    if(strlen(ARG[1])!=1) return '3';
    pin = arg_int(0);
    if(pin>7) return '5';
    if(ARG[1][0]=='0') cbi(*port,pin);
    else if(ARG[1][0]=='1') sbi(*port,pin);
    else return '4';
    return '0';
}

char RD_REG(volatile uint8_t *port) {
    if(strlen(ARG[0])!=1) return '3';
    if(ARG[0][0]>55||ARG[0][0]<48) return '4'; //Need the argument to be a number between '0'=48 and '7'=55
    // Make sure we don't want to access reserved registers
    if((port==&PINB)&&(ARG[0][0]=='6'||ARG[0][0]=='7')) return '7'; // XTAL
    if((port==&PINC)&&(ARG[0][0]=='6'||ARG[0][0]=='7')) return '7'; // RESET
    if((port==&PIND)&&(ARG[0][0]=='0'||ARG[0][0]=='1')) return '7'; // USART RX & DX
    uint8_t mask;
    mask = 1 << (ARG[0][0]-48);
    USART_SendRD();
    uint8_t val;
    val = *port;
    if ((val&mask)==0) USART_SendByte('0');
    else USART_SendByte('1');
    USART_SendByte('\n');
    return '0';
}

// --------- USART

void USART_SendRD(void){
    USART_SendByte('\n');
    USART_SendByte('R');
    USART_SendByte('D');
    USART_SendByte(':');
}



void USART_Init(void){
   // Set baud rate
   UBRRL = BAUD_PRESCALE;// Load lower 8-bits into the low byte of the UBRR register
   UBRRH = (BAUD_PRESCALE >> 8);
	 /* Load upper 8-bits into the high byte of the UBRR register
    Default frame format is 8 data bits, no parity, 1 stop bit
  to change use UCSRC, see AVR datasheet*/
  // Enable receiver and transmitter and receive complete interrupt
  UCSRB = ((1<<TXEN)|(1<<RXEN));
}

void USART_SendInt32(unsigned long int value){
    unsigned long int x;
    uint8_t i;
    unsigned long int n;
    uint8_t flag;
    flag = 0;
    x = value;
    for(i=0;i<10;i++) {
        n = x/(power_ten32(9-i));
        if (n!=0) flag=1;
        if (flag!=0) USART_SendByte(n+48);
        x-= n*power_ten32(9-i);}
}

void USART_SendInt(uint16_t value){
    uint16_t x;
    uint8_t i;
    uint16_t n;
    uint8_t flag;
    flag = 0;
    x = value;
    for(i=0;i<5;i++) {
        n = x/(power_ten(4-i));
        if (n!=0) flag=1;
        if (flag!=0) USART_SendByte(n+48);
        x-= n*power_ten(4-i);}
}

void USART_SendByte(uint8_t u8Data){
  while((UCSRA &(1<<UDRE)) == 0); // Wait until last byte has been transmitted
  UDR = u8Data; // Transmit data
}

void USART_SendMsg(char *msg_str){
    uint8_t i = 0;
    while((msg_str[i]!='\n')&&(msg_str[i]!='\0')&&(i<MAXSTR)){
    USART_SendByte(msg_str[i]);
    i++;
    }
    USART_SendByte(msg_str[i]);
}

uint8_t USART_ReceiveByte(){
  while((UCSRA &(1<<RXC)) == 0);
  return UDR;
}


// --------- COMMAND execution

void CMD_Clear(void){
    uint8_t i;
    for(i=0;i<SIZE_COMMAND;i++) COMMAND[i]='\0';
}

void CMD_Display(void){
    uint8_t i;
    USART_SendByte('\n');
    for(i=0;i<SIZE_CMD;i++) USART_SendByte(CMD[i]);
    for(i=0;i<N_ARG;i++) {
        USART_SendByte(':');
        USART_SendMsg(ARG[i]);}
    USART_SendByte('\n');
}

char CMD_Execute(void){
    char result;
    result = '6';
    if(strcmp(CMD,"WRPINB")==0) result = WR_REG(&PORTB);
    else if(strcmp(CMD,"WRPINC")==0) result = WR_REG(&PORTC);
    else if(strcmp(CMD,"WRPIND")==0) result = WR_REG(&PORTD);
    else if(strcmp(CMD,"DRPINB")==0) result = WR_REG(&DDRB);
    else if(strcmp(CMD,"DRPINC")==0) result = WR_REG(&DDRC);
    else if(strcmp(CMD,"DRPIND")==0) result = WR_REG(&DDRD);
    else if(strcmp(CMD,"RDPINB")==0) result = RD_REG(&PINB);
    else if(strcmp(CMD,"RDPINC")==0) result = RD_REG(&PINC);
    else if(strcmp(CMD,"RDPIND")==0) result = RD_REG(&PIND);
    else if(strcmp(CMD,"BLPINB")==0) result = BLINK(&PORTB);
    else if(strcmp(CMD,"BLPINC")==0) result = BLINK(&PORTC);
    else if(strcmp(CMD,"BLPIND")==0) result = BLINK(&PORTD);
    else if(strcmp(CMD,"MCWAIT")==0) result = WAIT();
    else if(strcmp(CMD,"WRPRTB")==0) result = WR_REG_BYTE(&PORTB);
    else if(strcmp(CMD,"WRPRTC")==0) result = WR_REG_BYTE(&PORTC);
    else if(strcmp(CMD,"WRPRTD")==0) result = WR_REG_BYTE(&PORTD);
    else if(strcmp(CMD,"WRDDRB")==0) result = WR_REG_BYTE(&DDRB);
    else if(strcmp(CMD,"WRDDRC")==0) result = WR_REG_BYTE(&DDRC);
    else if(strcmp(CMD,"WRDDRD")==0) result = WR_REG_BYTE(&DDRD);
    else if(strcmp(CMD,"SRESET")==0) result = reset();
    else if(strcmp(CMD,"RDEEPR")==0) result = EEPROM_RD();
    else if(strcmp(CMD,"WREEPR")==0) result = EEPROM_WR();
    else if(strcmp(CMD,"CLEEPR")==0) result = EEPROM_CLEAR();
    else if(strcmp(CMD,"RAPINC")==0) result = RD_ADC();
    else if(strcmp(CMD,"FPWMB1")==0) result = FPWM_Set(1);
    else if(strcmp(CMD,"FPWMB2")==0) result = FPWM_Set(2);
    else if(strcmp(CMD,"FPWMB3")==0) result = FPWM_Set(3);
    else if(strcmp(CMD,"SPWMB1")==0) result = FPWM_Stop(1);
    else if(strcmp(CMD,"SPWMB2")==0) result = FPWM_Stop(2);
    else if(strcmp(CMD,"SPWMB3")==0) result = FPWM_Stop(3);
    else if(strcmp(CMD,"WRINT0")==0) result = INT0_Set();
    else if(strcmp(CMD,"WRINT1")==0) result = INT1_Set();
    else if(strcmp(CMD,"STINT0")==0) result = INT0_Stop();
    else if(strcmp(CMD,"STINT1")==0) result = INT1_Stop();
    else if(strcmp(CMD,"IRDCD0")==0) result = INTIR_Set();
    else if(strcmp(CMD,"STDCD0")==0) result = INTIR_Stop();
    else if(strcmp(CMD,"WINSTR")==0) result = INSTR_Record();
    else if(strcmp(CMD,"LINSTR")==0) result = INSTR_List();
    else if(strcmp(CMD,"EINSTR")==0) result = INSTR_Execute();
    else if(strcmp(CMD,"RINSTR")==0) result = INSTR_Repeat();
    else if(strcmp(CMD,"UINSTR")==0) result = INSTR_Until();
    else if(strcmp(CMD,"WSQRB1")==0) result = SQUARE_Set(1);
    else if(strcmp(CMD,"WSQRB2")==0) result = SQUARE_Set(2);
    else if(strcmp(CMD,"WSQRB3")==0) result = SQUARE_Set(3);
    else if(strcmp(CMD,"SSQRB1")==0) result = FPWM_Stop(1);
    else if(strcmp(CMD,"SSQRB2")==0) result = FPWM_Stop(2);
    else if(strcmp(CMD,"SSQRB3")==0) result = FPWM_Stop(3);
    else if(strcmp(CMD,"SERVB1")==0) result = FPWM_Servo(1);
    else if(strcmp(CMD,"SERVB2")==0) result = FPWM_Servo(2);
    else if(strcmp(CMD,"INGOTO")==0) result = INSTR_Goto();
    else if(strcmp(CMD,"IFPINB")==0) result = INSTR_Ifpin(&PINB);
    else if(strcmp(CMD,"IFPINC")==0) result = INSTR_Ifpin(&PINC);
    else if(strcmp(CMD,"IFPIND")==0) result = INSTR_Ifpin(&PIND);
    else if(strcmp(CMD,"NOINST")==0) result = '0';
    return result;
}

char CMD_Decode(void){
    uint8_t i;
    uint8_t j;
    uint8_t i_arg;
    uint8_t length;
    char c;
    length = strlen(COMMAND);
    if(length<SIZE_CMD-1) return '1'; //ER1 COMMAND too short!
    for(i=0;i<SIZE_CMD;i++) CMD[i]=COMMAND[i];
    if(length==SIZE_CMD) {//No arguments
        c = CMD_Execute();
        return c;}
    //ARGUMENTS
    i=SIZE_CMD;
    CMD[i]='\0';
    if(COMMAND[i]!= '_') return '2'; //Invalid COMMAND
    i_arg = 0;
    while(COMMAND[i]=='_'&&i_arg<N_ARG&&i<length){
        j=0;
        i++;
        while((j<SIZE_ARG)&&(i<length)&&COMMAND[i]!='_') {
            ARG[i_arg][j] = COMMAND[i];
            i++;
            j++;}
        if(j!=SIZE_ARG) ARG[i_arg][j]='\0';
        i_arg++;
    }
    //CMD_Display();
    c = CMD_Execute();
    return c;
}


// ---------- Main program

char reset(void){
    // By default, all pins out
    DDRB=0b11111111;
    DDRC=0b00111111;
    DDRD=0b11111100;
    USART_Init();  // Initialise USART
    USART_SendMsg(msg0);
    is_IR_on = 0;
    INSTR_end = 0;
    USART_SendMsg(msg1);
    return '0';
}



int main(void){
    reset();
    uint8_t position;
    char error_code;
    while(1){
        position = 0;
        CMD_Clear();
        do {
        if ((UCSRA & (1<<RXC))) {
            COMMAND[position] = UDR;
            position++;
        }} while(COMMAND[position-1]!=13&&position<SIZE_COMMAND); //13 is ASCII character RETURN / newline
        //test_blink(1);
        //USART_SendMsg(COMMAND);
        COMMAND[position-1] = '\0'; //Termination character instead of RETURN
        error_code = CMD_Decode();
        if(error_code=='0')
            USART_SendMsg("OK\n");
        else {
            USART_SendByte('E');
            USART_SendByte('R');
            USART_SendByte(error_code);
            USART_SendByte('\n');}

   };
}



