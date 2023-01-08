/*
 * GccApplication1.c
 *
 * Created: 17.10.2022 15:07:04
 * Author : grzeg
 */ 

//miganie diod¹
/*
#include <util\delay.h>
#include <avr\pgmspace.h>
int main()
{
	DDRB |= (1<<PB0);
	while(1)
	{
		PORTB ^= (1<<PB0);
		_delay_ms(1000);
	}
}
*/

#include <stdbool.h>
#include <stdint.h>
#include "defines.h"
#include "hd44780.h"
#include "ESP8266.h"
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
volatile int i=0;
volatile int m=0;
int h=0;
volatile unsigned int licznik_przerwan=0;
volatile unsigned int stan=0;
unsigned int place=0;
volatile char temp =0;
ISR(TIMER0_OVF_vect)
{
	licznik_przerwan++;
}

ISR(PCINT2_vect)
{
	stan = 1;
	
}
void lcd_init()
{

	hd44780_init();				//Podstawowa inicjalizacja modu³u
	hd44780_outcmd(HD44780_CLR);	//Wyczyœæ pamiêæ DDRAM
	hd44780_wait_ready(1000);
	hd44780_outcmd(HD44780_ENTMODE(1, 0));	//Tryb autoinkrementacji AC
	hd44780_wait_ready(1000);
	hd44780_outcmd(HD44780_DISPCTL(1, 0, 0));	//W³¹cz wyœwietlacz, wy³¹cz kursor
	hd44780_wait_ready(1000);
}

void lcd_putchar(char c)
{
	static bool second_nl_seen;
	static uint8_t line=0;
	
	if ((second_nl_seen) && (c != '\n')&&(line==0))
	{//Odebrano pierwszy znak
		hd44780_wait_ready(40);
		hd44780_outcmd(HD44780_CLR);
		hd44780_wait_ready(1600);
		second_nl_seen=false;
	}
	if (c == '\n')
	{
		if (line==0)
		{
			line++;
			hd44780_outcmd(HD44780_DDADDR(64));	//Adres pierwszego znaku drugiej linii
			hd44780_wait_ready(1000);
		}
		else
		{
			second_nl_seen=true;
			line=0;
		}
	}
	else
	{
		hd44780_outdata(c);
		hd44780_wait_ready(40);
	}
}

void lcd_puttext_P(const char __flash *txt)
{
	char ch;
	while((ch=*txt))
	{
		lcd_putchar(ch);
		txt++;
	}
}

void lcd_puttext(const char __memx *txt)
{
	char ch;
	while((ch=*txt))
	{
		lcd_putchar(ch);
		txt++;
	}
}
void timer0_init()
{
	TCCR0B |=(1<<CS02);				//w³¹cz timer/prescaler 256
	TIMSK0 |= (1<<TOIE0);			//w³¹cz przerwania
	
}

int main()
{
	
	double temperatura=0;
	double temperatura_odczuwalna = 0;
	char tekst[20] ="witam w komputerze";
	DDRB |= (1<<PB0);
	//sprintf(tekst," dlicfhjhzba = %d",i);
	lcd_init();
	lcd_puttext(PSTR("Grzegorz Jur!\nWyswietlacz\n"));
	//hd44780_wait_ready(1600);
	//hd44780_outcmd(HD44780_CLR);
	timer0_init();
	//sei();
	//lcd_init();
	DDRD |= (1<<PD1);
	//DDRD |= (1<<PD6);
	//_delay_ms(10000);
	ustaw_w_tryb_at();
	//esp8866_resetuj();
	sei();

		
	_delay_ms(5000);
	polacz_z_router();
	_delay_ms(20000);
	//set_time();
	//_delay_ms(2000);
	//esp8866_resetuj();
	polacz_i_pobierz_pogoda();
	_delay_ms(3000);
	while(1) 
	{
		/*while(1)
		{
			PORTB ^= (1<<PB0);
			while (!(UCSR0A & (1<<UDRE0)));
			UDR0='a';
			_delay_ms(1000);
		}*/
	
	/*i=0;
	for (i = 0; i < 99; i++)
	{

		odbiorcza[i] = NULL;
		UDR=tekst[i];
	}
		for (i = 0; i < sizeof tekst; i++)
		{

			while (!(UCSRA & (1<<UDRE)));
			UDR=tekst[i];
		}*/

		PORTD |= (1<<PD7);
		//EICRA |= (1<<ISC11) | (1<<ISC10);
		//EIMSK |= (1<<INT1);
		PCICR |= (1<<PCIE2);
		PCMSK2 |= (1<<PCINT23);
		/*_delay_ms(10);
		if(!(PIND & (1<<PD7)))
		{
			stan++;
			if(stan == 4) stan=0;
		}*/
	
		
		/*_delay_ms(2000);
		for (i = 0; i < 900; i++)
		{

			while (!(UCSR0A & (1<<UDRE0)));
			UDR0=odbiorcza[i];
		}*/
		if(licznik_przerwan >= 27000)
		{
			polacz_i_pobierz_pogoda();
			_delay_ms(3000);
			licznik_przerwan = 0;
		}
		if(stan == 1)
		{
			place++;
			if(place == 4) place = 0;
			stan = 0;
			
		}
		//lcd_init();
		switch(place)
		{
			
		case 0:
			temperatura=pogoda2.temperatura-273.15;
			sprintf(tekst,"TEMPERATURA:\n%2.2f C\n",temperatura);
		
			//_delay_ms(2000);
			hd44780_outcmd(HD44780_CLR);
			hd44780_wait_ready(1600);
			lcd_puttext(tekst);
			_delay_ms(1000);
			break;
		//wyslij_czas();
		case 1:
			temperatura_odczuwalna=pogoda2.temperatura_odczuwalna-273.15;
			sprintf(tekst,"Odczuwalna:\n%2.2f C\n",temperatura_odczuwalna);
			hd44780_outcmd(HD44780_CLR);
			hd44780_wait_ready(1600);
			lcd_puttext(tekst);
			_delay_ms(1000);
			break;
		case 2:
			sprintf(tekst,"CISNIENIE:\n%2.2f hPa\n",pogoda2.cisnienie);
			hd44780_outcmd(HD44780_CLR);
			hd44780_wait_ready(1600);
			lcd_puttext(tekst);
			_delay_ms(1000);
			break;
		case 3:
			sprintf(tekst,"Wilgotnosc:\n%2.2f %% \n",pogoda2.wilgotnosc);	
			hd44780_outcmd(HD44780_CLR);
			hd44780_wait_ready(1600);
			lcd_puttext(tekst);
			_delay_ms(1000);
			break;
		}
		//_delay_ms(1000);
		//wyslij_czas();
		//sprintf(tekst,"%lu",dana_do_wysylki);
		//lcd_puttext(tekst);
		//PORTB ^= (1<<PB0);
		/*if(licznik_przerwan>=3907)
		{
			licznik_przerwan=0;
			i++;
			if(i>=60)
			{
				i=0;
				m++;
				hd44780_outcmd(HD44780_CLR);
				hd44780_wait_ready(1000);
			}
			if(m>=60)
			{
				h++;
				m=0;
				hd44780_outcmd(HD44780_CLR);
				hd44780_wait_ready(1000);
			}
			sprintf(tekst,"Czas \n%d:%d:%d\n",h,m,i);
			hd44780_outcmd(HD44780_HOME);
			hd44780_wait_ready(1000);
			lcd_puttext((tekst));
			hd44780_wait_ready(2000);		
		}*/
	}
		/*hd44780_outcmd(HD44780_HOME);
		sprintf(tekst,"liczba = %d",i);
		lcd_puttext((tekst));
		hd44780_wait_ready(1000);
		//_delay_ms(500);
	}*/
}