/*
 * main.hpp
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

#ifndef MAIN_HPP
#define MAIN_HPP

/*

MIG welding machine wire feeder controller to replace DC motor by step motor.

Inputs:
  //Fast Forward (from push button)
  Two One mode (from On/Off switch)
  Speed setting (PWM signal)

Outputs:
  Motor ENABLE (idle when Handle Switch is Off)
  Motor DIR (wire in/out depending on Two One mode)
  Motor STEP (frequency proportional to Speed and Current settings)

*/

/*
Running on internal 8 MHz oscillator

Oscillator Freq = 8 MHz
Main prescaler = 8

F_CPU 1 MHz

Timer0 1 MHz / 64 = 15625 Hz : STEP driver / 2 / x =  Hz max

Timer1 1 MHz / 16384 = ~61 Hz : 2-1 Mode driver /31 = ~2 Hz

PB0 : STEP out
PB1 : DIR out
PB2 : /ENABLE out
PB3 : 2-1 Mode input
PB4 : control voltage input
PB5 : /RESET input

Motor controller A4988

Timer0 step driver
Timer1 used to drive 2-1 mode

ADC2 : voltage proportional to PWM input

*/

#ifndef F_CPU
    #define F_CPU   (1000000)
#endif

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/iotn85.h>


enum
{
	TwoOne_Switch		= _BV(PB3),

	InEventsMask    = (TwoOne_Switch),

	MinSpeed = 200, // 2 m/min
	MaxSpeed = 253, // 11 m/min

	ADC_VRef_2v56   	= (_BV(REFS2) | _BV(REFS1)), // 2.56V internal VREF without external capacitor
	ADC_Left_Justified	= _BV(ADLAR),  //0x20 - left justified, so we can use ADCH as a 8 bit result
	ADC_InSpeed 		= _BV(MUX1),

	ADC_InSpeedMin = 77,  // 0.77V
	ADC_InSpeedMax = 220, // 2.20V

	Timer1_Freq = F_CPU / 16384, //61 Hz

	Timer1_C = Timer1_Freq / 4, //15 timer1 Top => ~ 4 Hz

	Timer1_A = Timer1_C / 6, //interrupt A phase
	Timer1_B = Timer1_A * 4, //interrupt B phase


	//gTime events
	TwoOne_Out     = (Timer1_Freq / Timer1_C) * 2,
	TwoOne_Cooloff = (Timer1_Freq / Timer1_C) * 3,
	TwoOne_In      = (TwoOne_Out + TwoOne_Cooloff) / 2,
};


inline void ADC_start()
{
    // ADC Start conversion
    ADCSRA |= _BV( ADSC );
}

inline void ADC_stop()
{
    // ADC Stop conversion
    ADCSRA &= ~_BV( ADSC );
}


inline void Motor_Off()
{
    PORTB |= _BV(PB2);
}

inline void Motor_On()
{
    PORTB &= ~_BV(PB2);
}

inline void Motor_SetSpeed(uint8_t v)
{
	OCR0A = ~v; //max speed if v = 255

	if(OCR0A < TCNT0)
	{
		TCNT0 = OCR0A - 1;
	}
}

inline void Wire_In()
{
    PORTB |= _BV(PB1);
}

inline void Wire_Out()
{
    PORTB &= ~_BV(PB1);
}

inline void InEvents_Enable()
{
    GIFR |= _BV(PCIF);     //clear int flag
    PCMSK |= InEventsMask; //set pin change interrupt mask
    GIMSK |= _BV(PCIE);    //enable pin change interrupt
}

inline void InEvents_Disable()
{
    GIMSK &= ~_BV(PCIE); //disable pin change interrupt
}

inline uint8_t InEvents_Read()
{
	return InEventsMask & PINB;
//	return InEventsMask & (~PINB); //inverted logic
}




#endif //MAIN_HPP
