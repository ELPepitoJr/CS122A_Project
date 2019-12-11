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
	SPCR = (1<<SPE)|(1<<SPIE);
	sei();
}
char SPI_SlaveReceive(void)
{
	/* Wait for reception complete */
	while(!(SPSR & (1<<SPIF)))
	;
	/* Return Data Register */
	return SPDR;
}

char numReceived;
ISR(SPI_STC_vect)
{
	numReceived = SPDR;
}
//------------------------------------------------------------------------------



int count;
char doOnce = 0;
char doItNever = 0;
char stepperCount;
int numphases = 3024;
char direction = 2;



enum lcdDisplay {Dinit, getInput, InstantDis, Open, Close} lcdstate;
enum stepper{Init1, A, AB, B, BC, C, CD, D, DA} motor;

const char* foo = "0123456789";
	
void stepperOpen()
{
		switch(motor)
		{
			case Init1:
				motor = A;
				break;
				
			case A:
				if(direction == 1 && numphases > 0)
				{
					motor = AB;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = DA;
				}
				else
				{
					motor = A;
				}
				break;
				
				
			case AB:
				if(direction == 1 && numphases > 0)
				{
					motor = B;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = A;
				}
				else
				{
					motor = AB;
				}
				break;
				
			case B:
				if(direction == 1 && numphases > 0)
				{
					motor = BC;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = AB;
				}
				else
				{
					motor = B;
				}
				break;
			
			case BC:
				if(direction == 1 && numphases > 0)
				{
					motor = C;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = B;
				}
				else
				{
					motor = BC;
				}
				break;
			
			case C:
				if(direction == 1 && numphases > 0)
				{
					motor = CD;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = BC;
				}
				else
				{
					motor = C;
				}
				break;
				
			case CD:
				if(direction == 1 && numphases > 0)
				{
					motor = D;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = C;
				}
				else
				{
					motor = CD;
				}
				break;
				
			case D:
				if(direction == 1 && numphases > 0)
				{
					motor = DA;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = CD;
				}
				else
				{
					motor = D;
				}
				break;
				
			case DA:
				if(direction == 1 && numphases > 0)
				{
					motor = A;
				}
				else if(direction == 0 && numphases > 0)
				{
					motor = D;
				}
				else
				{
					motor = DA;
				}
				break;
			
			default:
				motor = Init1;
				
		}
		switch(motor)
		{
			case Init1:
				break;
				
			case A:
				PORTA = 0x01;
				break;
				
			case AB:
				PORTA = 0x03;
				break;
				
			case B:
				PORTA = 0x02;
				break;
				
			case BC:
				PORTA = 0x06;
				break;
				
			case C:
				PORTA = 0x04;
				break;
			
			case CD:
				PORTA = 0x0C;
				break;
				
			case D:
				PORTA = 0x08;
				break;
				
			case DA:
				PORTA = 0x09;
				break;
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
			//sprintf(foo, "%d", numReceived);
			if(numReceived == 1)
			{
				lcdstate = InstantDis;
			}
			else
			{
				lcdstate = getInput;
			}
			break;
		
		case InstantDis:
			doOnce = 0;
			count = 0;
			direction = 1;
			doItNever = 0;
			lcdstate = Open;
			break;
		
		case Open:
			if(count < 4500)
			{
				doOnce = 1;
				++count;
				direction = 1;
				lcdstate = Open;
			}
			else
			{
				doOnce = 0; 
				doItNever = 0;
				numphases = 3024;
				direction = 0;
				count = 0;
				lcdstate = Close;
			}
			break;
			
		case Close:
			if(count < 4500)
			{
				doOnce = 1;
				++count;
				direction = 0;
				lcdstate = Close;
			}
			else
			{
				doOnce = 0;
				doItNever = 0;
				count = 0;
				numphases = 3024;
				direction = 2;
				numReceived = 0;
				lcdstate = getInput;
			}
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
				doItNever = 1;
			}
			break;
			
		case Open:
			if(doOnce == 1 && doItNever == 0)
			{
				//stepper motor function goes here
				//stepperOpen();
				doItNever = 1;
			}
			break;
		
		case Close:
			if(doOnce == 1 && doItNever == 0)
			{
				//stepper motor function goes here
				//stepperClose();
				doItNever = 1;
			}
			break;
			
	}
}

int main(void)
{
    /* Replace with your application code */
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	TimerSet(1);
	TimerOn();
	SPI_SlaveInit();
	
    while (1) 
    {
		display_tick();
		stepperOpen();
		
		while(!TimerFlag);
		TimerFlag = 0;
    }
}

