#ifndef BUTTON_ADC_H
#define BUTTON_ADC_H

#include <avr/io.h>  // Definice I/O registrů pro AVR

/* Makra ------------------------------------------------------------*/
#define UART_BAUD  9600       // Baudová rychlost pro UART
#define VREF 5000             // Referenční napětí v mV (5V)
#define BUTTON_THRESHOLD 100  // Minimální hodnota pro stisk tlačítka
#define NUM_KEYS 5            // Počet tlačítek
#define DEBOUNCE_DELAY 50    // Debounce zpoždění pro tlačítka

/* Tlačítka a jejich prahové hodnoty */
extern int adc_key_val[NUM_KEYS];

/* Prototypy funkcí -------------------------------------------------*/

/**
 * @brief Inicializace UART s definovanou baudovou rychlostí.
 */
void uart_init(void);

/**
 * @brief Odeslání jednoho znaku přes UART.
 * @param c Znak k odeslání.
 */
void uart_putc(char c);

/**
 * @brief Odeslání řetězce přes UART.
 * @param s Ukazatel na řetězec.
 */
void uart_puts(const char* s);

/**
 * @brief Inicializace ADC pro čtení z analogového pinu.
 */
void adc_init(void);

/**
 * @brief Získání aktuální hodnoty z ADC.
 * @return Hodnota ADC.
 */
uint16_t adc_read(void);

/**
 * @brief Detekce stisknutého tlačítka na základě ADC hodnoty.
 * @param input Hodnota z ADC.
 * @return Index tlačítka (0 až NUM_KEYS - 1) nebo -1, pokud není stisknuto.
 */
int get_key(unsigned int input);

/**
 * @brief Zpracování stisknutí tlačítka s debounce zpožděním.
 * @param old_key Poslední detekované tlačítko.
 * @return Index detekovaného tlačítka nebo -1.
 */
int process_button(int old_key);

/**
 * @brief Čtení napětí z ADC a převod na milivolty.
 * @return Napětí v milivoltech.
 */
uint32_t read_voltage(void);

#endif // BUTTON_ADC_H