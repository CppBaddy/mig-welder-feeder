/*
 * main.c
 *
 * Copyright (c) 2018 Yulay Rakhmangulov.
 *
 * Schematics and PCB design can be found here:
 *       https://easyeda.com/Yulay/nissan-cd-changer-emulator
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */



#include <stdint.h>
#include <stdbool.h>
#include <stdnoreturn.h>

#ifndef F_CPU
    #define F_CPU   8000000
#endif

#include <avr/io.h>
#include <avr/iotn85.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>


#include "main.hpp"

void setup();
void WDTimer_Init();

static uint8_t gPrevInput;

static volatile uint8_t gPwmReady;
static volatile uint8_t gTwoOneMode;

static volatile uint8_t gTime;

static uint8_t gPwmStart;
static volatile uint8_t gPwmValue;



/* Pin change interrupt handler */
ISR( PCINT0_vect )
{
	uint8_t val = InEvents_Read();

	uint8_t change = (val ^ gPrevInput);

	gPrevInput = val;

	if(change & PWM_input)
	{
		if(val & PWM_input)
		{
			gPwmStart = Timer1_Count();
		}
		else //save active duration
		{
			gPwmValue = Timer1_Count() - gPwmStart;

			InEvents_Disable();

			gPwmReady = 1;
		}
	}

	if(change & TwoOne_mode)
	{
		gTwoOneMode = (val & TwoOne_mode);
	}
}

/* Timer0 interrupt handler */
//ISR( TIMER0_OVF_vect )
//{
//}

/* Timer0 interrupt handler */
//ISR( TIMER0_COMPA_vect )
//{
//}

//ISR( TIMER0_COMPB_vect )
//{
//}

/* Timer1 interrupt handler */
//ISR( TIMER1_OVF_vect ) //eac
//{
//	++gTimer1;
//}

/* Timer1 interrupt handler */
//ISR( TIMER1_COMPA_vect ) //happens each 500uS or at 2000Hz
//{
//}

/* Timer1 interrupt handler */
//ISR( TIMER1_COMPB_vect )
//{
//}

// ADC interrupt service routine
//ISR( ADC_vect ) // this gets executed at about 2000 Hz
//{
//}

//watchdog timer interrupt as 2-1 mode driver
// prescaler 128K => 1 sec period
/* Watchdog interrupt handler */
ISR( WDT_vect )
{
    wdt_reset();

    if(gTwoOneMode)
	{
		++gTime;

		switch(gTime & 3)
		{
		case 0:
		case 1:
			Wire_Out();
			break;

		case 2:
			Wire_In();
			break;

		default:
			Wire_Out();
			++gTime;
			break;
		}
	}
}

int main( void )
{
//    WDTimer_Init();

    setup(); //setup ports

    InEvents_Enable();

	gTwoOneMode = TwoOne_mode & InEvents_Read();

	Wire_Out();
    Motor_SetSpeed(MinSpeed);

    for(;;)
    {
    	if(gPwmReady)
    	{
    		gPwmReady = 0;

    		gPwmValue += (MinSpeed - 20);

    		if(gPwmValue < MinSpeed)
    		{
    			gPwmValue = MinSpeed;
    		}

    		if(MaxSpeed < gPwmValue)
    		{
    			gPwmValue = MaxSpeed;
    		}

//    		gPwmValue = 250;

    		Motor_SetSpeed(gPwmValue);

    		InEvents_Enable();
    	}
    }

    return 0;
}


inline void setup()
{
    CLKPR = _BV(CLKPCE); //enable Clock Prescale Register write
    CLKPR = 0;           // change prescaler to 1, effectively set 8MHz system clock

    DDRB |= _BV(DDB2) | _BV(DDB1) | _BV(DDB0); /* set PBx to output */

    //PB5 - Reset (can be reused as separate pin later)

    //Timer0 step signal driver
    TCCR0A = _BV(COM0A0) | _BV(WGM01); //PB0 output, Mode 2: CTC
    TCCR0B = _BV(CS02) | _BV(CS00); //prescaler 1024
    OCR0A  = 0xff; //compare to TCNT0 to set output on PB0

    //Timer1 overflow each 512us
    TCCR1 = _BV(CS12) | _BV(CS10); //Mode 0; prescaler 16
    //TIMSK |= _BV(TOIE1); //enable overflow interrupt

    sei();
}

void WDTimer_Init()
{
    cli();

	wdt_disable();

    wdt_reset();

    // Use Timed Sequence for disabling Watchdog System Reset Mode if it has been enabled unintentionally.
    MCUSR = 0;

    WDTCR = _BV(WDCE) | _BV(WDE); // Enable configuration change.

    WDTCR = _BV(WDIF) | _BV(WDIE) // Enable Watchdog Interrupt Mode.
          | _BV(WDCE) | _BV(WDE)  // Disable Watchdog System Reset Mode if unintentionally enabled.
		  | _BV(WDP2) | _BV(WDP1);// Set Watchdog Timeout period to 1s.

    sei();
}

