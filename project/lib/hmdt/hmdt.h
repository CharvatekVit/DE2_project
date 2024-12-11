#ifndef HMDT_H
#define HMDT_H

#include <stdint.h>

// ADC kanál pro senzor vlhkosti půdy
#define ANALOG_IN 0  

// Kalibrační hodnoty vlhkosti
extern int maxHodnota;
extern int minHodnota;

// Deklarace funkcí
void ADC_init(void);
int analogRead(uint8_t channel);
int nactiVlhkost(void);
int map(int x, int in_min, int in_max, int out_min, int out_max);

#endif // HMDT_H
