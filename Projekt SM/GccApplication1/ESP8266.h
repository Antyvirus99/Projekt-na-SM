/*
 * ESP8266.h
 *
 * Created: 14.03.2017 19:09:39
 *  Author: Donat
 */ 


#ifndef ESP8266_H_
#define ESP8266_H_

extern volatile char odbiorcza[900];
extern unsigned long dana_do_wysylki;

extern void wyzeruj_dane_po_wyslaniu_na_serwer (void);
extern void esp8866_resetuj (void);
extern void ustaw_w_tryb_at(void);
extern void ustaw_w_tryb_normalny(void);
extern char wyslij_na_serwer (void);
extern void polacz_z_NTP (void);
extern void rozlacz (void);
extern unsigned long polacz_i_pobierz_czas_UNIX (void);
extern void polacz_z_router (void);
extern void wyslij_komenda_bajty(void);
extern void wyslij_czas(void);
extern unsigned long polacz_i_pobierz_pogoda (void);
extern void set_time (void);
extern void get_time(void);
extern struct pogoda pogoda2;
struct pogoda
{
	volatile double temperatura;
	volatile double temperatura_odczuwalna;
	volatile double cisnienie;
	volatile double wilgotnosc;
};

#endif /* ESP8266_H_ */