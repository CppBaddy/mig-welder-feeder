#ifndef MAIN_H
#define MAIN_H

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
Main prescaler = 1

CPU Frequency 8 MHz

Timer0 8MHz / 1024 = ~8 KHz : STEP driver / 2 / 2 = 2KHz max

Timer1 8MHz / 16 = ~500 KHz : PWM measure

PB0 : STEP out
PB1 : DIR out
PB2 : /ENABLE out
PB3 : 2-1 Mode input
PB4 : PWM control input
PB5 : /RESET input

Motor controller A4988

Timer0 step driver
Timer1 used to measure PWD settings
WDTimer used to drive 2-1 mode

*/

#ifndef F_CPU
    #define F_CPU   (8000000/8)
#endif

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>


enum
{
    InputAveraging      = 3,

	TwoOne_mode		= _BV(PB3),
	PWM_input		= _BV(PB4),

	InEventsMask    = (PWM_input | TwoOne_mode),

    ModeIdle,
    ModeFastFeed,
	ModePowerOn,

	MinSpeed = 200,
	MaxSpeed = 250,
	// 253 -> 1.2 KHz
	// 250 -> 646 Hz

};


inline void Timer0_Enable()
{
//    PRR &= ~_BV(PRTIM0); //clock enable
//    TIFR0 |= _BV(OCF0A);
//    TIMSK0 |= _BV(OCIE0A);
}

inline void Timer0_Disable()
{
//    TIMSK0 &= ~_BV(OCIE0A);
//	PRR |= _BV(PRTIM0); //clock disable
}

inline void Timer1_Enable()
{
//    PRR &= ~_BV(PRTIM1); //clock enable
//    TIFR1 |= _BV(OCF1A);
//    TIMSK1 |= _BV(OCIE1A);
}

inline void Timer1_Disable()
{
//    TIMSK1 &= ~_BV(OCIE1A);
//    PRR |= _BV(PRTIM1); //clock disable
}

extern volatile uint8_t gTimer1;

inline uint8_t Timer1_Count()
{
	return TCNT1;
}

inline void Timer1_Reset()
{
	TCNT1 = 0;
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
    GIFR |= _BV(PCIF); //clear int flag
    PCMSK |= InEventsMask; //set pin change interrupt mask
    GIMSK |= _BV(PCIE);  //enable pin change interrupt
}

inline void InEvents_Disable()
{
    GIMSK &= ~_BV(PCIE); //disable pin change interrupt
}

inline uint8_t InEvents_Read()
{
	return InEventsMask & (~PINB); //inversion logic
}

inline void SetLED(bool v)
{
	if(v)
	{
		PORTB |= _BV(PB1);
	}
	else
	{
		PORTB &= ~_BV(PB1);
	}
}

inline void ToggleLED()
{
	PINB |= _BV(PB1);
}



#endif //MAIN_H
