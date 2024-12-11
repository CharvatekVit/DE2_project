#include "button_adc.h"
#include <util/delay.h>

#define NUM_KEYS 5
#define DEBOUNCE_DELAY 50 // Debounce delay v ms
#define VREF 5000         // Referenční napětí v mV

// ADC hodnoty odpovídající jednotlivým tlačítkům
int adc_key_val[NUM_KEYS] = {550, 650, 700, 800, 900};

void uart_init(void) {
    unsigned int ubrr = F_CPU / 16 / UART_BAUD - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);             
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_putc(char c) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

void uart_puts(const char* s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void adc_init(void) {
    ADMUX |= (1 << REFS0);                  // Referenční napětí AVCC
    ADMUX &= ~(1 << MUX0);                  // ADC0 jako vstup
    ADCSRA |= (1 << ADEN);                  // Povolit ADC
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
}

uint16_t adc_read(void) {
    ADCSRA |= (1 << ADSC);                  // Spustit konverzi
    while (ADCSRA & (1 << ADSC));           // Čekat na dokončení
    return ADC;                             // Vrátit výsledek
}

int get_key(unsigned int input) {
    for (int k = 0; k < NUM_KEYS; k++) {
        if (input < adc_key_val[k]) {
            return k;
        }
    }
    return -1; // Pokud není tlačítko detekováno
}

int process_button(int old_key) {
    uint16_t button_value = adc_read();
    int detected_key = get_key(button_value);

    // Pokud je nové tlačítko stisknuté
    if (detected_key != old_key) {
        _delay_ms(DEBOUNCE_DELAY); // Debounce delay
        uint16_t rechecked_value = adc_read();
        int rechecked_key = get_key(rechecked_value);

        // Pokud je tlačítko stále stejné po debounce
        if (rechecked_key == detected_key) {
            while (get_key(adc_read()) == detected_key) {
                // Čeká, dokud tlačítko není uvolněno
            } // <- Tento WHILE zajistí, že se tlačítko zpracuje jen jednou
            return detected_key;
        }
    }
    return old_key;
}

