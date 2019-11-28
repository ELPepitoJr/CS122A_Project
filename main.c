/*
 * Lab3 part1.c
 *
 * Created: 10/16/2019 1:57:11 PM
 * Author : Salvador
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <avr/eeprom.h>
#include "bit.h"
#include "keypad.h"
#include "lcd.h"
#define DDR_SPI PORTB
#define DD_MOSI PORTB5
#define DD_SCK PORTB7
#define DD_MISO PORTB6
#define DD_SS PORTB4

//SPI
//------------------------------------------------------------------------------
void SPI_MasterInit(void)
{
	/* Set MOSI and SCK and SS output, all others input */
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	SREG = 0x80;
}
void SPI_MasterTransmit(char cData)
{
	/* Start transmission */
	SPDR = cData;
	DDR_SPI = (0<<DD_SS);
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));
	DDR_SPI = (1<<DD_SS);
}

void SPI_SlaveInit(void)
{
	/* Set MISO output, all others input */
	DDR_SPI = (1<<DD_MISO);
	/* Enable SPI */
	SPCR = (1<<SPE);
}
char SPI_SlaveReceive(void)
{
	/* Wait for reception complete */
	while(!(SPSR & (1<<SPIF)))
	;
	/* Return Data Register */
	return SPDR;
}
//------------------------------------------------------------------------------

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


char cups = '1';
char timesFilled = '0';
int count;
char half = '5';
float amount;
char dot = '.';
char doOnce = 0;
char doItNever = 0;
char stepperCount;
int numphases = 1024;
char direction;

int getInputKeypad()
{
	int bool = 0; // mostly for if A is pressed
	char temp = '0';
	temp = GetKeypadKey();
	
	if(temp == 'A')
	{
		//instantly fills the bowl, does not count as a refill
		bool = 1;
	}
	else if (temp == 'B')
	{
		//opens the latch, does not close until C is pressed
		bool = 2;
	}
	else if(temp == 'C')
	{
		//closes the latch
		bool = 3;
	}
	else if(temp == '0')
	{
		//reset the timesFilled char
		bool = 8;
		timesFilled = 0;
	}
	else if(temp == '1')
	{
		bool = 4;
		cups = '0'; //half a cup
		amount = 0.5;
		if(timesFilled < 2)
		{
			timesFilled += 1;
		}
	}
	else if(temp == '2')
	{
		bool = 5;
		cups = '1'; //1 cup
		amount = 1.0;
		if(timesFilled < 2)
		{
			timesFilled += 1;
		}
	}
	else if(temp == '3')
	{
		bool = 6;
		cups = '1'; //1.5 cups
		amount = 1.5;
		if(timesFilled < 2)
		{
			timesFilled += 1;
		}
	}
	else if(temp == 'D')
	{
		//gives treat only
		bool = 7;
	}
	
	return bool;
	
}

enum lcdDisplay {Dinit, getInput, InstantDis, Open, Close, HalfCup, OneCup, OneHalfCup, Display} lcdstate;
enum stepper{Init1, A, AB, B, BC, C, CD, D, DA} motor;
enum stepper1{Init11, A1, AB1, B1, BC1, C1, CD1, D1, DA1} motor1;
	
void stepperOpen()
{
	while(numphases > 0)
	{
		switch(motor)
		{
			case Init1:
				motor = A;
				break;
				
			case A:
				PORTB = 0x01;
				motor = AB;
				break;
				
			case AB:
				PORTB = 0x03;
				motor = B;
				break;
				
			case B:
				PORTB = 0x02;
				motor = BC;
				break;
			
			case BC:
				PORTB = 0x06;
				motor = C;
				break;
			
			case C:
				PORTB = 0x04;
				motor = CD;
				break;
				
			case CD:
				PORTB = 0x0C;
				motor = D;
				break;
				
			case D:
				PORTB = 0x08;
				motor = DA;
				break;
				
			case DA:
				PORTB = 0x09;
				motor = A;
				break;
			
			default:
				motor = Init1;
				break;
				
		}
		numphases--;
	}
}

void stepperClose()
{
	while(numphases > 0)
	{
		switch(motor1)
		{
			case Init11:
			motor1 = A1;
			break;
			
			case A1:
			PORTB = 0x01;
			motor1 = DA1;
			break;
			
			case AB1:
			PORTB = 0x03;
			motor1 = A1;
			break;
			
			case B1:
			PORTB = 0x02;
			motor1 = AB1;
			break;
			
			case BC1:
			PORTB = 0x06;
			motor1 = B1;
			break;
			
			case C1:
			PORTB = 0x04;
			motor1 = BC1;
			break;
			
			case CD1:
			PORTB = 0x0C;
			motor1 = C1;
			break;
			
			case D1:
			PORTB = 0x08;
			motor1 = CD1;
			break;
			
			case DA1:
			PORTB = 0x09;
			motor1 = D1;
			break;
			
			default:
			motor = Init11;
			break;
			
		}
		numphases--;
	}
}

void display_tick()
{
	
	switch(lcdstate)
	{
		case Dinit:
			lcdstate = getInput;
			break;
			
		case getInput:
			if(getInputKeypad() == 1)
			{
				//fills bowl instantly
				lcdstate = InstantDis;
			}
			else if(getInputKeypad() == 2)
			{
				//opens latch
				lcdstate = Open;	
			}
			else if(getInputKeypad() == 3)
			{
				//closes the latch
				lcdstate = Close;
			}
			else if(getInputKeypad() == 4)
			{
				lcdstate = HalfCup;
			}
			else if(getInputKeypad() == 5)
			{
				lcdstate = OneCup;
			}
			else if(getInputKeypad() == 6)
			{
				lcdstate = OneHalfCup;
			}
			else
			{
				lcdstate = getInput;
			}
			break;
		
		case InstantDis:
			if(count < 200)
			{
				doOnce = 1;
				++count;
				lcdstate = InstantDis;
			}
			else
			{
				doOnce = 0;
				count = 0;
				doItNever = 0;
				lcdstate = Open;
			}
			break;
		
		case Open:
			if(count < 700)
			{
				doOnce = 1;
				++count;
				lcdstate = Open;
			}
			else
			{
				doOnce = 0;
				doItNever = 0;
				numphases = 1024;
				count = 0;
				lcdstate = Close;
			}
			break;
			
		case Close:
			if(count < 700)
			{
				doOnce = 1;
				++count;
				lcdstate = Close;
			}
			else
			{
				doOnce = 0;
				count = 0;
				numphases = 1024;
				lcdstate = Display;
			}
			break;
			
		case HalfCup:
			if(count < 200)
			{
				doOnce = 1;
				++count;
				lcdstate = HalfCup;
			}
			else
			{
				doOnce = 0;
				count = 0;
				lcdstate = Display;
			}
			break;
			
		case OneCup:
			if(count < 200)
			{
				doOnce = 1;
				++count;
				lcdstate = OneCup;
			}
			else
			{
				doOnce = 0;
				count = 0;
				lcdstate = Display;
			}
			break;
			
		case OneHalfCup:
			if(count < 200)
			{
				doOnce = 1;
				++count;
				lcdstate = OneHalfCup;
			}
			else
			{
				doOnce = 0;
				count = 0;
				lcdstate = Display;
			}
			break;
			
		case Display:
			lcdstate = getInput;
			break;

		default:
			lcdstate = Dinit;
			break;
	}
	
	switch(lcdstate)
	{
		case Dinit:
			break;
			
		case getInput:
			break;
			
		case InstantDis:
			if(doOnce == 1 && doItNever == 0)
			{
				LCD_DisplayString(1, "Dispensing Now");
				doItNever = 1;
			}
			break;
			
		case Open:
			//LCD_DisplayString(1, "Opening Now");
			if(doOnce == 1 && doItNever == 0)
			{
				//stepper motor function goes here
				LCD_DisplayString(1, "Opening Now");
				stepperOpen();
				doItNever = 1;
			}
			break;
		
		case Close:
			//LCD_DisplayString(1, "Closing Now");
			if(doOnce == 1 && doItNever == 0)
			{
				//stepper motor function goes here
				LCD_DisplayString(1, "Closing Now");
				stepperClose();
				doItNever = 1;
			}
			break;
			
		case HalfCup:
			if(doOnce == 1 && doItNever == 0)
			{
				LCD_DisplayString(1, "Half a Cup has Been selected");
				doItNever = 1;
			}
			break;
			
		case OneCup:
			if(doOnce == 1 && doItNever == 0)
			{
				LCD_DisplayString(1, "One Cup has Been selected");
				doItNever = 1;
			}
			break;
			
		case OneHalfCup:
			if(doOnce == 1 && doItNever == 0)
			{
				LCD_DisplayString(1, "1.5 cups has Been selected");
				doItNever = 1;
			}
			break;
			
		case Display:
			LCD_DisplayString(1, "Amount:   Cups  Times filled: ");
			LCD_Cursor(8);
			if( (amount < 1.0 && amount > 0.0) || (amount > 1.0 && amount < 2.0) )
			{
				LCD_WriteData(cups);
				LCD_Cursor(9);
				LCD_WriteData(dot);
				LCD_Cursor(10);
				LCD_WriteData(half);
			}
			else
			{
				LCD_WriteData(cups);
			}
			
			LCD_Cursor(31);
			LCD_WriteData(timesFilled);
			doItNever = 0;
			break;
	}
}

int main(void)
{
    /* Replace with your application code */
	DDRC = 0xF0; PORTC = 0x0F;  //keypad init
	DDRA = 0xFF; PORTA = 0x00;  //LCD control line
	DDRD = 0xFF; PORTD = 0x00;  //LCD bus lines
	DDRB = 0xFF; PORTB = 0x00;
	TimerSet(10);
	TimerOn();
	LCD_init();
	LCD_ClearScreen();
	
    while (1) 
    {
		display_tick();
		
		while(!TimerFlag);
		TimerFlag = 0;
    }
}

