#include <avr/io.h>
#include <util/delay.h>
#include "hmdt.h"

int maxHodnota = 0;
int minHodnota = 1024;

// Inicializace ADC
void ADC_init(void) {
    ADMUX = (1 << REFS0); // Referenční napětí AVCC
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // ADC enable, dělič 64
}

// Čtení analogové hodnoty z ADC
int analogRead(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); // Vybrání ADC kanálu
    ADCSRA |= (1 << ADSC); // Zahájení převodu
    while (ADCSRA & (1 << ADSC)); // Čekání na dokončení převodu
    return ADC;
}

// Přemapování hodnot
int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Funkce pro načtení vlhkosti
int nactiVlhkost(void) {
    int namerenaHodnota = analogRead(ANALOG_IN);
    return map(namerenaHodnota, minHodnota, maxHodnota, 0, 100);
}
