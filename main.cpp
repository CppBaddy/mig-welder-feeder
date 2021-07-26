/*
 * main.cpp
 *
 * Copyright (c) 2021 Yulay Rakhmangulov.
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


#include "main.hpp"

#include <stdnoreturn.h>

#include <avr/interrupt.h>


void setup();

static volatile uint8_t gPrevInput;

static volatile uint8_t gTwoOneMode;

static volatile uint8_t gTime;

static volatile uint8_t gSpeedSet;



/* Pin change interrupt handler */
ISR( PCINT0_vect )
{
	uint8_t val = InEvents_Read();

	uint8_t change = (val ^ gPrevInput);

	gPrevInput = val;

	if(change & TwoOne_Switch)
	{
		gTwoOneMode = (val & TwoOne_Switch);

		if(!gTwoOneMode)
		{
			Wire_Out();
			gTime = 0;
		}
	}
}

// Timer1 interrupt handler
ISR( TIMER1_COMPA_vect ) //each 1s
{
	if(gTwoOneMode)
	{
		++gTime;

		if(gTime < TwoOne_Out)
		{
			Wire_Out();
		}
		else if(gTime < TwoOne_In)
		{
			Wire_In();
		}
		else
		{
			Wire_Out();
			gTime = 0;
		}
	}

	ADC_start();
}

// Timer1 interrupt handler
ISR( TIMER1_COMPB_vect ) //each 1s, phase shifted from TIMER1_COMPA_vect
{
	ADC_start();
}

// ADC interrupt service routine
ISR( ADC_vect )
{
	uint8_t val = ADCH;

	if(gSpeedSet)
	{
		val >>= 1; //divide by 2

		//sliding average
		gSpeedSet = (gSpeedSet >> 1) + val;
	}
	else
	{
		gSpeedSet = val; //initialize
	}

	Motor_SetSpeed(gSpeedSet);
}

int main( void )
{
    setup(); //setup ports

    InEvents_Enable();

	gTwoOneMode = (TwoOne_Switch & InEvents_Read());

	Motor_On();
	Wire_Out();
    Motor_SetSpeed(MinSpeed);

    for(;;)
    {
        //TODO sleep, keep Timers and ADC On. Allow pin change and timer interrupts to wake up 
    }

    return 0;
}


inline void setup()
{
    DDRB |= _BV(DDB2) | _BV(DDB1) | _BV(DDB0); /* set PBx to output */

    //PB5 - Reset (can be reused as separate pin later)

    // ADC Voltage Reference: internal 2.56V
    // ADC High Speed Mode: Off
    ADMUX = ( ADC_Left_Justified | ADC_InSpeed | ADC_VRef_2v56 );

    // ADC Enabled
    // ADC Clock frequency: prescaler 16, F_OSC/16 = 62.5KHz, conversion time 208us
    ADCSRA |= _BV( ADEN ) | _BV( ADIE ) | _BV( ADPS2 );

    //Timer0 step signal driver
    TCCR0A = _BV(COM0A0) | _BV(WGM01); //PB0 output, Mode 2: CTC
    TCCR0B = _BV(CS01); //prescaler 8
    OCR0A  = 0xff; //compare to TCNT0 to toggle output on PB0

    //Timer1 CTC
    TCCR1 = _BV(CTC1) | _BV(CS13) | _BV(CS12) | _BV(CS11) | _BV(CS10); //Mode 2; CTC prescaler 16384
    TIMSK |= _BV(OCIE1A) | _BV(OCIE1B); //enable Compare Match A and B interrupts
    OCR1A  = Timer1_A;  //to get interrupt A
    OCR1B  = Timer1_B;  //to get interrupt B
    OCR1C  = Timer1_C;  //CTC reload value

    sei();
}

