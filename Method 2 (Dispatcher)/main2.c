#pragma config FPLLIDIV = DIV_10 // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_15 // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1 // System PLL Output Clock Divider (PLL Divide by 1)
// DEVCFG1
#pragma config FNOSC = PRIPLL // Oscillator Selection Bits (Primary Osc w/PLL)
#pragma config FSOSCEN = OFF // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF // Internal/External Switch Over (Disabled)
#pragma config POSCMOD = XT // Primary Oscillator Configuration (XT osc mode)
#pragma config OSCIOFNC = OFF // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_1 // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/1)
#pragma config FCKSM = CSDCMD // (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576 // Watchdog Timer Postscaler (1:1048576)
#pragma config FWDTEN = OFF // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
// DEVCFG0
#pragma config DEBUG = OFF // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx2 // (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF // Program Flash Write Protect (Disable)
#pragma config BWP = OFF // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF // Code Protect (Protection Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#pragma interrupt InterruptHandler ipl1 vector 0
#include <xc.h>
#include <p32xxxx.h>
#include<plib.h>

// Variable Initialization

int state=0, number=0;
int microseconds=0,seconds=0,minutes=0;
int firststate= 0;
int mscounter=0;
int debouncer =0;
int pause_resume = 0;
int rst = 0;

void initialize(){
    // E pins are the segment pins 
    mPORTESetPinsDigitalOut(BIT_0|BIT_1|BIT_2|BIT_3|BIT_4|BIT_5|BIT_6);
    mPORTESetBits(BIT_0|BIT_1|BIT_2|BIT_3|BIT_4|BIT_5|BIT_6);
    mPORTEToggleBits(BIT_0|BIT_1|BIT_2|BIT_3|BIT_4|BIT_5|BIT_6);
    
    // F/D Digit Pins
    mPORTFSetPinsDigitalOut(BIT_2|BIT_3);
    mPORTDSetPinsDigitalOut(BIT_0|BIT_8);
   
    mPORTFSetBits(BIT_2);
    PORTFbits.RF3=0;
    PORTDbits.RD8=0;
    PORTDbits.RD0=0;
    
    // LED setup to make sure the button is working
    mPORTFSetPinsDigitalOut(BIT_0);
    mPORTFSetBits(BIT_0) ;
    mPORTFToggleBits(BIT_0);
       
    // Button Setup
    mPORTDSetPinsDigitalIn(BIT_10);
    mPORTDSetPinsDigitalIn(BIT_3);
    
        
    // Set Timers
    OpenTimer1(T1_ON|T1_PS_1_8,0x186A); //60HZ
    mT1SetIntPriority(1);
    INTEnableSystemSingleVectoredInt();
    mT1IntEnable(1);
    
}
void reset() {
	if (rst) {
		 microseconds=0;
           seconds=0;
           minutes=0;
           firststate=0;
           rst = 0;
           pause_resume = 0;
	}
}
int  sevensegment(int c) {
    switch (c)
	{
	case 1: return ~0x06;
	case 2: return ~0x5b;
	case 3: return ~0x4f;
	case 4: return ~0x66;
	case 5: return ~0x6d;
	case 6: return ~0x7d;
	case 7: return ~0x07;
	case 8: return ~0x7f;
	case 9: return ~0x6f;
	case 0: return ~0x3f;
	}
}       // Function to return sevensegment numbers
void display(){
    PORTE = sevensegment(number); 
    switch(state)
        {   
            case 0:{ state=1; mPORTFToggleBits(BIT_2|BIT_3); 
            if(firststate)
                number = seconds/10;
            else 
                number = microseconds/10;
            } break;
            
            case 1:{  state=2;  PORTFbits.RF3=0;PORTDbits.RD8=1; 
            if(firststate)
                number = seconds%10;
            else 
                number = microseconds%10;}   break;
                
            case 2:{ state=3; mPORTDToggleBits(BIT_0|BIT_8); 
            if(firststate)
                number = minutes/10;
            else 
                number = seconds/10; }  break;
            case 3: {state =0; PORTFbits.RF2=1;PORTDbits.RD0=0;
            if(firststate)
                number = minutes%10;
            else 
                number = seconds%10;}  break;
        }
    ReRunMe(1);
} 
void secondsminutes(){
    if (!pause_resume)
		microseconds++;
    if(microseconds>59)
    {
        seconds++;
        microseconds=0;
    }
    else if(seconds>59)
    {
        minutes++;
        seconds=0;
        firststate=1;
    }

ReRunMe(4);
}          // Function to increment seconds and minutes


unsigned char SwitchActionTaken = 0 ,SwitchActionTaken2=0;


void CheckSwitch2()
{
    int val = LATDbits.LATD10;
    if(!val && !SwitchActionTaken)
    {pause_resume = ~pause_resume; 
    
    SwitchActionTaken = 1;
    }    
    else if (val)
        SwitchActionTaken  = 0;
    
    ReRunMe(1);
}
void CheckSwitch1()
{
    int val = LATDbits.LATD3;
    if(!val)
    {rst = 1;
    reset();
    
    
    SwitchActionTaken2 = 1;
    }    
    else if (val)
    {
        
        SwitchActionTaken2 = 0;
    }
    
    ReRunMe(1);
}

void InterruptHandler(void) {
     
    if(mT1GetIntFlag())
    {
     FinishDelay();
     mT1ClearIntFlag();
    }
    
}

void main() {
    
    InitMulti();
    QueTask(initialize);
    QueTask(display);
    QueTask(CheckSwitch1);
    QueTask(CheckSwitch2);
    QueTask(secondsminutes);
    while(1)
    {   
        Dispatch();
    }
}


