#include <avr/io.h>
#define F_CPU 9600000
#include <util/delay.h>
#include <avr/interrupt.h>
#include "limits.h"

#include "pinout.h"


// #define UINT16_MAX USHRT_MAX

// Counter stating the number of cycle in the PWM
#define MAX_COUNTER 80

volatile uint16_t ButtonPressBuffer = 0;
volatile uint16_t ButtonPressReader = 0;

// Define the colors register
struct EColors {
	uint8_t Red;
	uint8_t Green;
	uint8_t Blue;
} Colors;

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
	if(Colors.Blue > 0x00 && Colors.Red == MAX_COUNTER && Colors.Green == 0x00) {
		Colors.Blue-=1;
	}
	if(Colors.Blue == MAX_COUNTER && Colors.Red < MAX_COUNTER && Colors.Green == 0x00) {
		Colors.Red+=1;
	}

	//Fade from green to blue
	if(Colors.Green > 0x00 && Colors.Blue == MAX_COUNTER && Colors.Red == 0x00) {
		Colors.Green-=1;
	}
	if(Colors.Green == MAX_COUNTER && Colors.Blue < MAX_COUNTER && Colors.Red == 0x00) {
		Colors.Blue+=1;
	}

	//Fade from red to green
	if(Colors.Red > 0x00 && Colors.Green == MAX_COUNTER && Colors.Blue == 0x00) {
		Colors.Red-=1;
	}
	if(Colors.Red == MAX_COUNTER && Colors.Green < MAX_COUNTER && Colors.Blue == 0x00) {
		Colors.Green+=1;
	}

	uint8_t i;
	for(i=0; i<adc; ++i)
	{
		_delay_ms(1);
	}
}

void Light( const int adc)
{
	Colors.Red = Colors.Blue = MAX_COUNTER;
	Colors.Green = 0;
}

void Red( const int adc)
{
	Colors.Red = MAX_COUNTER;
	Colors.Green = Colors.Blue = 0;
}

void Green( const int adc)
{
	Colors.Green = MAX_COUNTER;
	Colors.Red = Colors.Blue = 0;
}

void Blue( const int adc)
{
	Colors.Blue = MAX_COUNTER;
	Colors.Green = MAX_COUNTER;
	Colors.Red = MAX_COUNTER;
}

int main(void)
{
	init();

	Colors.Red = MAX_COUNTER;
	Colors.Green = 0;
	Colors.Blue = 0;

	DisplayMode currentMode = EGREEN;

	void (*func)(int);
	func = &Green;

    while (1)
    {
		int adc = adc_read();

		if(ButtonPressReader > 6000)
		{
			Colors.Red = MAX_COUNTER;
			Colors.Green = MAX_COUNTER;
			Colors.Blue = 0;

			_delay_ms(2000);

			ButtonPressReader = 0;

			// Increment the mode
			if(currentMode == EBLUE)
			{
				currentMode = RAINBOW;
			}
			else
			{
				currentMode = (DisplayMode)((int)currentMode +1);
			}

			// Change the function based on the mode
			switch(currentMode)
			{
			case RAINBOW:
				Colors.Red = MAX_COUNTER;
				Colors.Green = 0;
				Colors.Blue = 0;
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

	if( Colors.Red <= pwm_counter)
	{
		portb_buffer &= ~(_BV(RED));
	}

	if( Colors.Green <= pwm_counter)
	{
		portb_buffer &= ~(_BV(GREEN));
	}

	if( Colors.Blue <= pwm_counter)
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