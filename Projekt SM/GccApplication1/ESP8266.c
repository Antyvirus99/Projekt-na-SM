/*
 * ESP8266.c
 *
 * Created: 14.03.2017 19:09:23
 *  Author: Donat
 */ 


#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>


#include <string.h>
#include <stdlib.h>

#include "ESP8266.h"
#include <stdio.h>
const __flash char juz_polaczono[] = "ALREADY CONNECTED\r\n";
const __flash char polaczono_z_serwerem[] = "CONNECT\r\n\r\nOK\r\n";
const __flash char zapisano_na_serwerze[] = "zapisano_dane";
const __flash char odebrano_bajty[] = "bytes\r\n";
const __flash char komenda_wlacz_robocza[] = "AT+CWMODE_CUR=3\r\n";
const __flash char komenda_polacz_do_serwer[] = "AT+CIPSTART=\"TCP\",\"192.168.1.51\",80\r\n";
const __flash char komdena_wyslij_ilosc_do_serwer[] = "AT+CIPSEND=";
const __flash char komenda_wyslij_adresI[] = "GET http://192.168.1.51/JUR/php/notpad_do_TCPIP.php?dana=";
const __flash char komenda_wyslij_adresII[] = " HTTP/1.1\r\nHost: 192.168.1.51\r\n\r\n";
int liczba_errorow = 0;
char dane[15];
char dane2[10];
char timestamp[10];
int ilosc_znakow;
unsigned long dana_do_wysylki = 0;

const __flash char komenda_polacz[] = "AT+CIPSTART=\"UDP\",\"time1.google.com\",123";//"AT+CIPSTART=\"UDP\",\"ntp1.tp.pl\",123";
const __flash char komenda_polacz_pogoda[] = "AT+CIPSTART=\"TCP\",\"api.openweathermap.org\",80";
const __flash char komenda_bajty_pogoda[] = "AT+CIPSEND=112";
const __flash char zapytanie_o_pogoda[] = "GET /data/2.5/weather?q=knurow&APPID=cf24c94ac2550178e2ea4e970cd0f416 HTTP/1.1\r\nHost: api.openweathermap.org\r\n\r\n";
const __flash char komenda_rozlacz[] = "AT+CIPCLOSE";
const __flash char komenda_bajty[] = "AT+CIPSEND=48";
const __flash char komenda_wyslij[] = "AT+CMGS=\"796163560\"";
volatile char odbiorcza[900];
const __flash char komenda_set_time[] = "AT+CIPSNTPCFG=1,1";
const __flash char komenda_get_time[] = "AT+CIPSNTPTIME?";
const __flash char komenda_SZUKANA_OK[] = "\r\nOK\r\n";
const __flash char komenda_szukana_IPD[] = "+IPD,48:";
const __flash char komenda_szukana_IPD2[] = "+IPD,";
const __flash char komenda_SZUKANA_kontynuuj[] = "\r\nOK\r\n> ";
const __flash char komenda_polacz_z_router[] = "AT+CWJAP_CUR=\"[wifi]\",\"[haslo]\"";
volatile unsigned int opoznienie = 0;
unsigned long czas_odebrany = 0;
unsigned long czas_UNIX = 0;
char * wynik;

const __flash char kom_temperatura[] = "\"temp\":";
const __flash char kom_temperatura_odczuwalna[] = "\"feels_like\":";
const __flash char kom_cisnienie[] = "\"pressure\":";
const __flash char kom_wilgotnosc[] = "\"humidity\":";
struct pogoda pogoda2;


volatile unsigned int index_odebrany = 0;
signed int index_szukany = 0;
unsigned int index_zapisu = 0;
unsigned int index_tekstu = 0;

ISR (USART_RX_vect)
{
	odbiorcza[index_odebrany] = UDR0;
	index_odebrany++;
}

void czekaj_ms (int czas)
{
	czas/=10;
	
	while ((index_odebrany == 0) && (czas > 0))		//czekaj 2 sekundy lub az odbierze informacje zwrotna
	{
		_delay_ms(10);
		czas--;						
	}
	_delay_ms(500);
}
char potwierdz_zapis_na_serwerze (void)
{
	if(memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
					index_odebrany,									//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					zapisano_na_serwerze,								//szukany tekst
					strlen_P(zapisano_na_serwerze)					//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					) != NULL
	) return 1;		
	else return 0;
}
char potwierdz_polaczenie_z_serwerem (void)
{
	if(memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
					index_odebrany,									//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					polaczono_z_serwerem,								//szukany tekst
					strlen_P(polaczono_z_serwerem)					//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					) != NULL
	) return 1;
	else return 0;	
}
char potwierdz_przesyl_adresu (void)
{
	if(memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
					index_odebrany,									//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					odebrano_bajty,								//szukany tekst
					strlen_P(odebrano_bajty)					//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					) != NULL
	) return 1;	
	else return 0;
}
char potwierdz_przeslane_bajty (void)
{
	if(memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
					index_odebrany,									//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					komenda_SZUKANA_kontynuuj,								//szukany tekst
					strlen_P(komenda_SZUKANA_kontynuuj)					//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
					) != NULL
	) return 1;
	else return 0;
}
char znajdz_ok (void)
{

	if(memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
			index_odebrany,									//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
			komenda_SZUKANA_OK,								//szukany tekst
			strlen_P(komenda_SZUKANA_OK)					//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
			) != NULL
	) return 1;
	else return 0;
	
}
void polacz_z_router (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_polacz_z_router); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_polacz_z_router[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
}

void wyslij_komenda_bajty (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_bajty); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_bajty[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
}

void wyslij_komenda_bajty_POGODA (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_bajty_pogoda); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_bajty_pogoda[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
}


void esp8866_resetuj (void)
{
	PORTD &=~ (1<<PD6);
	_delay_ms(100);
	PORTD |= (1<<PD6);
	_delay_ms(8000);
	
}
void pobierz_czas (void)
{
	czas_odebrany = 0;
	czas_odebrany = czas_odebrany +  16777216ul*(* wynik); //16e6;
	wynik ++;
	czas_odebrany = czas_odebrany + 65536ul*(* wynik) ;//16e4;
	wynik ++;
	czas_odebrany = czas_odebrany + 256ul*(* wynik);//16e2;
	wynik ++;
	czas_odebrany = czas_odebrany + (* wynik);
	
	czas_UNIX = czas_odebrany - 2208988800;
	sprintf(timestamp,"%lu",czas_UNIX);
	
	
}
void wyslij_czas (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = czas_UNIX;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
	czas_UNIX = czas_UNIX >> 8;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = czas_UNIX;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
	czas_UNIX = czas_UNIX >> 8;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = czas_UNIX;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
	czas_UNIX = czas_UNIX >> 8;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = czas_UNIX;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
	
}

void wyzeruj_dane_po_wyslaniu_na_serwer (void)
{
		_delay_ms(2000);
		rozlacz();
		dana_do_wysylki = 0;
		czas_UNIX = 0;
		_delay_ms(10000);

}
void wyslij_bajty (void)
{
	index_odebrany = 0;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x1b;
	for (index_tekstu = 0; index_tekstu < 47; index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = 0;
	}
}

void wyslij_api (void)
{
	index_odebrany = 0;
	for (index_tekstu = 0; index_tekstu < 112; index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = zapytanie_o_pogoda[index_tekstu];
	}
}

void polacz_z_NTP (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_polacz); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_polacz[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
}
void polacz_z_POGODA (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_polacz_pogoda); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_polacz_pogoda[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
}
void rozlacz (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_rozlacz); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_rozlacz[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
	
}

unsigned long polacz_i_pobierz_czas_UNIX (void)
{
	polacz_z_NTP();
	
	czekaj_ms(1000);
			
	if((		memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
	index_odebrany,										//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	komenda_SZUKANA_OK,									//szukany tekst
	strlen_P(komenda_SZUKANA_OK)						//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	)
	)
	||
	(			memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
	index_odebrany,										//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	juz_polaczono,										//szukany tekst
	strlen_P(juz_polaczono)								//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	)
	))
	{
		wyslij_komenda_bajty();
		czekaj_ms(2000);
		if(		memmem_P(	(const void *)odbiorcza,								//tablica w której bêdziemy szukaæ
		index_odebrany,											//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
		komenda_SZUKANA_kontynuuj,								//szukany tekst
		strlen_P(komenda_SZUKANA_kontynuuj)						//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
		)
		)
		{

					
			wyslij_bajty();
			czekaj_ms(3000);
			wynik = memmem_P(	(const void *)odbiorcza,						//tablica w której bêdziemy szukaæ
			index_odebrany,									//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
			komenda_szukana_IPD,							//szukany tekst
			strlen_P(komenda_szukana_IPD)					//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
			);
			
			
			if (wynik != NULL)
			{
				//... a jeœli nie znajdzie IPD??? - brakuje reakcji
				wynik += 48; // ustaw wskaznik na date wyslania czasu unix
				pobierz_czas();
				
				
					wyslij_czas();
				//tutaj przenios³em wysy³anie +++ i wszystko dzia³a OK
				//_delay_ms(500);
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				_delay_ms(1000);			//niezbêdny zgodnie z dokumentacj¹
					
				rozlacz();
				czekaj_ms(1000);
				PORTD ^= (1<<PD7);
				
				
				wynik = 0;
					for (index_tekstu = 0; index_tekstu < strlen_P(komenda_rozlacz); index_tekstu++)
					{
						while (!(UCSR0A & (1<<UDRE0)));
						UDR0 = timestamp[index_tekstu];
					}
					while (!(UCSR0A & (1<<UDRE0)));
					UDR0 = 0x0d;
					while (!(UCSR0A & (1<<UDRE0)));
					UDR0 = 0x0a;
				return czas_UNIX;
				
			}
				
			else
			{
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				_delay_ms(1000);			//niezbêdny zgodnie z dokumentacj¹
				
				rozlacz();
				czekaj_ms(1000);
				PORTD ^= (1<<PD7);
				//wyslij_czas();
				//czas_UNIX = 4732;
				wynik = 0;
				return 66;
			}



		}
		else return 66;	
				
	}
	else return 66;			
}



void ustaw_w_tryb_at(void)
{
	UCSR0A |= (1<<U2X0);
	UBRR0L = 15; // predkosc 115200 bps
	UCSR0B |= (1<<RXEN0) | (1<<TXEN0);// wlacz transmisje i odbior
	//transmisja asynchroniczna nie trzeba
	//parzystosci nie trzeba
	//bitow stop nie trzeba
	UCSR0C |= (1 << UCSZ01) | (1 <<UCSZ00); //wysylaj 8 bitow
	UCSR0B |= (1<<RXCIE0);//wlacz przerwania odbioru
}


void convert_pogoda(void)
{
	char * pointer;
	char temp[10];
	pointer=memmem_P((const void *) wynik,index_odebrany,kom_temperatura,strlen_P(kom_temperatura));
	pointer+=(sizeof(kom_temperatura)-1);
	for(int i=0;i<=9;i++)
	{
		temp[i]=0;
		if(*pointer == ',') break;
		temp[i]=*pointer;
		pointer++;
		
	}
	pogoda2.temperatura = atof(temp);
	
	pointer=memmem_P((const void *) wynik,index_odebrany,kom_cisnienie,strlen_P(kom_cisnienie));
	pointer+=(sizeof(kom_cisnienie)-1);
	for(int i=0;i<=9;i++)
	{
		temp[i]=0;
		if(*pointer == ',') break;
		temp[i]=*pointer;
		pointer++;
		
	}
	pogoda2.cisnienie = atof(temp);
	
	pointer=memmem_P((const void *) wynik,index_odebrany,kom_wilgotnosc,strlen_P(kom_wilgotnosc));
	pointer+=(sizeof(kom_wilgotnosc)-1);
	for(int i=0;i<=9;i++)
	{
		temp[i]=0;
		if(*pointer == ',') break;
		temp[i]=*pointer;
		pointer++;
		
	}
	pogoda2.wilgotnosc = atof(temp);
	
	pointer=memmem_P((const void *) wynik,index_odebrany,kom_temperatura_odczuwalna,strlen_P(kom_temperatura_odczuwalna));
	pointer+=(sizeof(kom_temperatura_odczuwalna)-1);
	for(int i=0;i<=9;i++)
	{
		temp[i]=0;
		if(*pointer == ',') break;
		temp[i]=*pointer;
		pointer++;
		
	}
	pogoda2.temperatura_odczuwalna = atof(temp);
}


unsigned long polacz_i_pobierz_pogoda (void)
{
	polacz_z_POGODA();
	
	czekaj_ms(3000);
	
	if((		memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
	index_odebrany,										//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	komenda_SZUKANA_OK,									//szukany tekst
	strlen_P(komenda_SZUKANA_OK)						//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	)
	)
	||
	(			memmem_P(	(const void *) odbiorcza,						//tablica w której bêdziemy szukaæ
	index_odebrany,										//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	juz_polaczono,										//szukany tekst
	strlen_P(juz_polaczono)								//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
	)
	))
	{
		wyslij_komenda_bajty_POGODA();
		czekaj_ms(5000);
		if(		memmem_P(	(const void *)odbiorcza,								//tablica w której bêdziemy szukaæ
		index_odebrany,											//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
		komenda_SZUKANA_kontynuuj,								//szukany tekst
		strlen_P(komenda_SZUKANA_kontynuuj)						//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
		)
		)
		{

			
			wyslij_api();
			czekaj_ms(5000);
			
			wynik = memmem_P(	(const void *)odbiorcza,						//tablica w której bêdziemy szukaæ
			index_odebrany,									//ile tekstu z tej tablicy ma przeszukaæ    (-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
			komenda_szukana_IPD2,							//szukany tekst
			strlen_P(komenda_szukana_IPD2)					//d³ugoœæ tekstu szukanego					(-1 bo do tablicy dodawany jest znak koñca tekstu, którym jest bajt o wartoœci zero)
			);
			
			
			convert_pogoda();
			if (wynik != NULL)
			{
				wynik+=360;
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				_delay_ms(1000);			//niezbêdny zgodnie z dokumentacj¹
				
				rozlacz();
				czekaj_ms(3000);
				PORTD ^= (1<<PD7);
				
				/*while (!(UCSRA & (1<<UDRE)));
				UDR = pogoda2.temperatura;
				while (!(UCSRA & (1<<UDRE)));
				UDR = 0x0d;
				while (!(UCSRA & (1<<UDRE)));
				UDR = 0x0a;*/
				//wynik = 0;
				/*for (index_tekstu = 0; index_tekstu < 900; index_tekstu++)
				{
					while (!(UCSRA & (1<<UDRE)));
					UDR = wynik[index_tekstu];
				}
				while (!(UCSRA & (1<<UDRE)));
				UDR = 0x0d;
				while (!(UCSRA & (1<<UDRE)));
				UDR = 0x0a;*/
				return 100;
				
			}
			
			else
			{
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				while (!(UCSR0A & (1<<UDRE0)));
				UDR0 = '+';
				_delay_ms(1000);			//niezbêdny zgodnie z dokumentacj¹
				
				rozlacz();
				czekaj_ms(1000);
				PORTD ^= (1<<PD7);

				wynik = 0;
				return 66;
			}



		}
		else return 66;
		
	}
	else return 66;
}
void set_time (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_set_time); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_set_time[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
}
void get_time (void)
{
	index_odebrany = 0; //Zeby odpowiedz byla pakowana na poczatek buforu
	for (index_tekstu = 0; index_tekstu < strlen_P(komenda_get_time); index_tekstu++)
	{
		while (!(UCSR0A & (1<<UDRE0)));
		UDR0 = komenda_get_time[index_tekstu];
	}
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0d;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = 0x0a;
}