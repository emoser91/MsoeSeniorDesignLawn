/*Senior Design Lawn Mower
 * Eric Moser
 *
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include "MSOE_I2C/delay.h"
#include "MSOE_I2C/bit.h"
#include "MSOE_I2C/lcd.h"

//PORTS
//inputs: PB1:fwd/rev switch, PB2:blade switch, PB3:start mower switch

//Outputs: PC2:fwd/rev relay, PC3:blade relay, PD2: alternator relay,
//PD3: start mower contactor

//ADC: PC0 battery voltage

//Timer: PD5 or PD6 throttle servo

//LCD PC5-clk, PC4-dat


uint16_t BatteryVoltage;//second variable to store distance data
int main(void)
{
	//Direction Switch in and relay out
	DDRC|=(1<<PORTC2);
	DDRB&=(~(1<<PORTB1));
	PORTB|=(1<<PORTB1);
	uint8_t inputdirection;

	//Blade Switch in and relay out
	DDRC|=(1<<PORTC3);
	DDRB&=(~(1<<PORTB2));
	PORTB|=(1<<PORTB2);
	uint8_t inputblade;

	//starter switch in and relay out
	DDRD|=(1<<PORTD3);
	DDRB&=(~(1<<PORTB3));
	PORTB|=(1<<PORTB3);
	uint8_t inputstart;

	//seat sensor and power relay out
	DDRB|=(1<<PORTB5);//power relay
	DDRB&=(~(1<<PORTB4));//seat sensor
	PORTB|=(1<<PORTB4);
	uint8_t inputseat;

	//alternator
	DDRD|=(1<<PORTD2);

	//Servo fast pwm
	DDRD|=(1<<PORTD6);
	DDRD|=(1<<PORTD5);//wave output
	TCCR0A=(1<<COM0B1)|(1<<WGM01)|(1<<WGM00);//fast pwm mode and clear on compare
	TCCR0B=(1<<CS02)|(1<<WGM02);//prescale 64
	OCR0A=200;//top =2k hz

	//ADC for battery voltage
	ADMUX=(1<<REFS0);
	ADCSRA=(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN)|(1<<ADIE);
	ADCSRA|=(1<<ADSC);//conversion start
	PORTC=PORTC|(0b00000001);//turning off pull-up resistor
	sei();//turning on global interrupts
	float volts;//decimal point number

	//variables
	int change=0;

	//Setting up LCD
	lcd_init();//lcd enable
	lcd_clear();
	lcd_home();

	while(1)
	{
		//seat sensor
		inputseat=PINB;
		inputseat=inputseat&0b00010000;
		
		if(inputseat==0)//seat sensor
		{
			PORTB&=(~(1<<PORTB5));//power relay on
			lcd_home();
			lcd_printf("Volt ADCW = %d",BatteryVoltage);//printing distance ADWC
			volts=(float)BatteryVoltage;
			volts=volts*5/1023;//calculating voltage
			volts=volts*11;
			lcd_goto_xy(0,1);
			lcd_printf("v = ");
			lcd_goto_xy(4,1);
			lcd_print_float(volts);//printing voltage

			//Direction
			inputdirection=PINB;
			inputdirection=inputdirection&0b00000010;
			if(inputdirection==0)//forward case
			{
				PORTC|=(1<<PORTC2);//fwd
				//PORTC&=(~(1<<PORTC2));

				//Blade
				inputblade=PINB;
				inputblade=inputblade&0b00000100;
				if(inputblade==0)
				{
					PORTC&=(~(1<<PORTC3));//blade on
					//PORTC|=(1<<PORTC3);//blade off - debug
				}
				else
				{
					PORTC|=(1<<PORTC3);//blade off
					//PORTC&=(~(1<<PORTC3));//blade on - debug
				}

			}
			
			else
			{
			//reverse case
				//PORTC|=(1<<PORTC2);
				PORTC&=(~(1<<PORTC2));//rev
			}

			//starting case
			inputstart=PINB;
			inputstart=inputstart&0b00001000;
			if(inputstart==0)
			{
				//change=0;
				if(change==0)
				{
					OCR0B=99; //set start throttle pos
					PORTD&=(~(1<<PORTD3));//starter relay on
					PORTD&=(~(1<<PORTD2));//alternator relay on
					delay_ms(4000); //delay to wait for IC engine start
					//PORTD|=(1<<PORTD3);// IC engine with starter off
					change=1;
				}
				
				if(change==1)
				{
					PORTD|=(1<<PORTD3);// IC engine with starter off
					//OCR0B=99;
					//blade voltage increase
					inputblade=PINB;
					inputblade=inputblade&0b00000100;
				
					if(inputblade==0)
					{
						OCR0B=120;
					}
					
					else
					{
						OCR0B=99;
					}
					
					if(volts<35)
						{
							OCR0B=OCR0B+1;
						}

						if(volts>38)
						{
							OCR0B=OCR0B-1;
						}
					}
		}//if
		
		else
			{
			PORTD|=(1<<PORTD3);// IC engine with starter off
			PORTD|=(1<<PORTD2);//alternator relay off
			OCR0B=60;//throttle off to kill engine
			change=0;
			}//else
				
		}//if statement for seat
		
		else
		{
			PORTB|=(1<<PORTB5);//power relay off
		}
		
	}//while
}//main

ISR(ADC_vect)//ADC interrupt for when a conversion stops
{
	BatteryVoltage=ADCW;//wring ADC result to a global
	ADMUX=ADMUX&(~(1<<MUX0));//turn on ADC for PC0
	ADCSRA|=(1<<ADSC);//starting a new ACD conversion
}//isr
