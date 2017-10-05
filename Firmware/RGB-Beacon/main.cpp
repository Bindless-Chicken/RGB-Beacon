#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "limits.h"

// Pinout
//	IN
#define RED PB4
#define GREEN PB0
#define BLUE PB2
//	OUT
#define POTENTIOMETER PB3
#define BUTTON PB1

#define UINT16_MAX USHRT_MAX

// Counter stating the number of cycle in the PWM
#define MAX_COUNTER 80

volatile uint8_t pR, pG, pB;
volatile uint16_t ButtonPressBuffer = 0;
volatile uint16_t ButtonPressReader = 0;

void init(void)
{
	// Init pins
	DDRB = 0x00;
	DDRB = _BV(RED) | _BV(GREEN) | _BV(BLUE);

	// Init ADC
	ADMUX |= (1 << MUX0) | (1 << MUX1);
	ADMUX |= (1 << ADLAR);
	ADCSRA |= (1 << ADPS1) | (1 << ADPS0) | (1 << ADEN);

	// Set interupt for timer
	TCCR0B |= _BV(CS00);
	TIMSK0 |= _BV(TOIE0);

	sei();
}

int adc_read (void)
{
	// Start the conversion
	ADCSRA |= (1 << ADSC);

	// Wait for it to finish - blocking
	while (ADCSRA & (1 << ADSC));

	return ADCH;
}

typedef enum E_DisplayMode{
	RAINBOW,
	ERED,
	EGREEN,
	EBLUE,
	LIGHT
}DisplayMode;

void Rainbow( const int adc)
{
	if(pB > 0x00 && pR == MAX_COUNTER && pG == 0x00) {
		pB-=1;
	}
	if(pB == MAX_COUNTER && pR < MAX_COUNTER && pG == 0x00) {
		pR+=1;
	}

	//Fade from green to blue
	if(pG > 0x00 && pB == MAX_COUNTER && pR == 0x00) {
		pG-=1;
	}
	if(pG == MAX_COUNTER && pB < MAX_COUNTER && pR == 0x00) {
		pB+=1;
	}

	//Fade from red to green
	if(pR > 0x00 && pG == MAX_COUNTER && pB == 0x00) {
		pR-=1;
	}
	if(pR == MAX_COUNTER && pG < MAX_COUNTER && pB == 0x00) {
		pG+=1;
	}

	for(uint8_t i=0; i<adc; ++i)
	{
		_delay_ms(1);
	}
}

void Light( const int adc)
{
	pR = pB = MAX_COUNTER;
	pG = 0;
}

void Red( const int adc)
{
	pR = MAX_COUNTER;
	pG = pB = 0;
}

void Green( const int adc)
{
	pG = MAX_COUNTER;
	pR = pB = 0;
}

void Blue( const int adc)
{
	pB = MAX_COUNTER;
	pG = MAX_COUNTER;
	pR = MAX_COUNTER;
}

int main(void)
{
	init();

	pR = MAX_COUNTER;
	pG = 0;
	pB = 0;

	DisplayMode currentMode = EGREEN;

	void (*func)(int);
	func = &Green;

    while (1)
    {
		int adc = adc_read();

		if(ButtonPressReader > 6000)
		{
			pR = MAX_COUNTER;
			pG = MAX_COUNTER;
			pB = 0;

			_delay_ms(2000);

			ButtonPressReader = 0;

			// Increment the mode
			if(currentMode == EBLUE)
			{
				currentMode = RAINBOW;
			}
			else
			{
				currentMode = DisplayMode((int)currentMode +1);
			}

			// Change the function based on the mode
			switch(currentMode)
			{
			case RAINBOW:
				pR = MAX_COUNTER;
				pG = 0;
				pB = 0;
				func = &Rainbow;
			break;
			case ERED:
				func = &Red;
			break;
			case EGREEN:
				func = &Green;
			break;
			case EBLUE:
				func = &Blue;
			break;
			case LIGHT:
				func = &Red;
			break;
			}
		}

		func(adc);
    }
}

volatile uint8_t pwm_counter = 0x00;


ISR(TIM0_OVF_vect)
{
	uint8_t portb_buffer= 0xFF;
	if( pR <= pwm_counter)
	{
		portb_buffer &= ~(_BV(RED));
	}

	if( pG <= pwm_counter)
	{
		portb_buffer &= ~(_BV(GREEN));
	}

	if( pB <= pwm_counter)
	{
		portb_buffer &= ~(_BV(BLUE));
	}

	pwm_counter+=1;

	if(pwm_counter == MAX_COUNTER)
	{
		pwm_counter = 0;
	}

	if(PINB & _BV(BUTTON))
	{
		if( ButtonPressBuffer < UINT16_MAX)
			ButtonPressBuffer+=1;


		PORTB = 0x00;
	}
	else
	{
		PORTB = portb_buffer;

		if(ButtonPressBuffer > 0)
			ButtonPressReader = ButtonPressBuffer;

		ButtonPressBuffer = 0;
	}
}