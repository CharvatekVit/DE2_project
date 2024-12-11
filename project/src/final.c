

/* Defines -----------------------------------------------------------*/
// Pin definitions for buttons
#define BUTTON_PIN 1       // Button for switching modes
#define UP_BUTTON_PIN 2    // Button to increase value
#define DOWN_BUTTON_PIN 3  // Button to decrease value
#define ANALOG_IN 0        // ADC channel for moisture sensor

// LED pin definitions
#define WATERING_LED 4     // LED for watering
#define DRYING_LED 5       // LED for drying
#define HEATING_LED 6      // LED for heating
#define COOLING_LED 7      // LED for cooling

#define SENSOR_ADR 0x5c
#define SENSOR_HUM_MEM 0
#define SENSOR_TEMP_MEM 2
#define SENSOR_CHECKSUM 4

/* Moisture sensor calibration ---------------------------------------*/
#define ADC_MIN 527        // ADC value for dry sensor (0%)
#define ADC_MAX 250        // ADC value for fully immersed sensor (100%)
#define DEBOUNCE_DELAY 5   // Debounce delay for button (5 ms)
#define CURSOR_TIMEOUT 100 // Timeout for cursor disappearance (10 seconds)

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <twi.h> 
#include "timer.h"
#include <oled.h>
#include <util/delay.h>

/* Global variables --------------------------------------------------*/
// Variables for display mode and cursor management
volatile uint8_t display_mode = 0;    // 0 = Normal mode, 1 = Set mode
volatile uint8_t set_mode_option = 0; // 0 = Moisture, 1 = Temperature
int set_moisture = 50;                // Target soil moisture (0–100%)
int set_temp = 25;                    // Target temperature (0–60°C)
uint8_t cursor_visible = 0;           // Cursor visibility state
uint16_t cursor_timer = 0;            // Timer for cursor disappearance

// Structure to hold DHT12 sensor data
struct DHT_values_structure {
    uint8_t hum_int;
    uint8_t hum_dec;
    uint8_t temp_int;
    uint8_t temp_dec;
    uint8_t checksum;
} dht12;

/* Function prototypes -----------------------------------------------*/
void ADC_init(void);
int analogRead(uint8_t channel);
int readHumidity(void);
void ledControl(int humidity, int temperature);
uint8_t isButtonPressed(uint8_t pin);
void displayCursor(void);
void clearCursor(void);
uint8_t readDHT12(void);

/* Main --------------------------------------------------------------*/
int main(void) {
    char string[10]; // Buffer for converting integers to strings

    // Initialize display, ADC, and TWI (I2C)
    oled_init(OLED_DISP_ON);
    oled_clrscr();
    ADC_init();
    twi_init();

    // Set pin modes for LEDs and buttons
    DDRD |= (1 << WATERING_LED) | (1 << DRYING_LED) | (1 << HEATING_LED) | (1 << COOLING_LED);
    DDRD &= ~((1 << BUTTON_PIN) | (1 << UP_BUTTON_PIN) | (1 << DOWN_BUTTON_PIN));
    PORTD |= ((1 << BUTTON_PIN) | (1 << UP_BUTTON_PIN) | (1 << DOWN_BUTTON_PIN));

    while (1) {
        int humidity = readHumidity();    // Read soil moisture
        uint8_t dht_status = readDHT12(); // Read temperature from DHT12

        // Button press handling
        if (isButtonPressed(BUTTON_PIN)) {
            cursor_visible = 1; // Show cursor
            cursor_timer = 0;   // Reset cursor timer
            clearCursor();
            if (display_mode == 1) {
                set_mode_option = !set_mode_option; // Toggle between moisture and temperature setting
            } else {
                display_mode = !display_mode; // Toggle between normal and set mode
            }
        }

        // Handle "Set Mode"
        if (display_mode == 1) {
            cursor_visible = 1;  // Show cursor
            cursor_timer = 0;    // Reset cursor timer

            if (set_mode_option == 0) {  // Adjust soil moisture
                if (isButtonPressed(UP_BUTTON_PIN) && set_moisture < 100) set_moisture++;
                if (isButtonPressed(DOWN_BUTTON_PIN) && set_moisture > 0) set_moisture--;
            } else {  // Adjust temperature
                if (isButtonPressed(UP_BUTTON_PIN) && set_temp < 60) set_temp++;
                if (isButtonPressed(DOWN_BUTTON_PIN) && set_temp > 0) set_temp--;
            }
        }

        // Control LEDs based on conditions
        ledControl(humidity, dht12.temp_int);

        // Display soil moisture
        oled_gotoxy(0, 0);
        oled_puts("Soil: ");
        itoa(humidity, string, 10);
        oled_puts(string);
        oled_puts(" %   ");

        // Display air temperature
        oled_gotoxy(0, 1);
        oled_puts("Temp: ");
        if (dht_status) {
            itoa(dht12.temp_int, string, 10);
            oled_puts(string);
            oled_puts(" C   ");
        } else oled_puts("ER");

        // Display target soil moisture
        oled_gotoxy(0, 2);
        oled_puts("Set Soil: ");
        if (set_moisture < 10) oled_puts("  "); 
        else if (set_moisture < 100) oled_puts(" ");
        itoa(set_moisture, string, 10);
        oled_puts(string);
        oled_puts(" %");

        // Display target temperature
        oled_gotoxy(0, 3);
        oled_puts("Set Temp: ");
        if (set_temp < 10) oled_puts("  ");
        else if (set_temp < 100) oled_puts(" ");
        itoa(set_temp, string, 10);
        oled_puts(string);
        oled_puts(" C");

        // Cursor management
        if (cursor_visible) {
            displayCursor();
            cursor_timer++;
            if (cursor_timer > CURSOR_TIMEOUT) {
                clearCursor();
                cursor_visible = 0; // Hide cursor after timeout
            }
        }

        oled_display();
        _delay_ms(100); // Main loop delay
    }
}

/* Button press detection --------------------------------------------*/
uint8_t isButtonPressed(uint8_t pin) {
    static uint8_t lastState = 1;
    uint8_t currentState = (PIND & (1 << pin)) ? 1 : 0;
    if (currentState == 0 && lastState == 1) {
        _delay_ms(DEBOUNCE_DELAY);
        lastState = 0;
        return 1;
    } else if (currentState == 1) lastState = 1;
    return 0;
}

/* Display and clear cursor ------------------------------------------*/
void displayCursor(void) {
    if (set_mode_option == 0) oled_gotoxy(15, 2);
    else oled_gotoxy(15, 3);
    oled_putc('_');
}

void clearCursor(void) {
    oled_gotoxy(15, 2);
    oled_putc(' ');
    oled_gotoxy(15, 3);
    oled_putc(' ');
}

uint8_t readDHT12(void) {
    twi_start();
    if (twi_write((SENSOR_ADR << 1) | TWI_WRITE) != 0) return 0;
    twi_write(0x5c);
    twi_stop();
    twi_start();
    twi_write((SENSOR_ADR << 1) | TWI_READ);
    dht12.hum_int = twi_read(1);
    dht12.hum_dec = twi_read(1);
    dht12.temp_int = twi_read(1);
    dht12.temp_dec = twi_read(1);
    dht12.checksum = twi_read(0);
    twi_stop();
    uint8_t sum = dht12.hum_int + dht12.hum_dec + dht12.temp_int + dht12.temp_dec;
    return (sum == dht12.checksum);
}





/* Initialize ADC for analog reading ---------------------------------*/
void ADC_init(void) {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

int analogRead(uint8_t channel) {
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

/* Read soil moisture ------------------------------------------------*/
int readHumidity(void) {
    int value = analogRead(ANALOG_IN);
    if (value > ADC_MIN) value = ADC_MIN;
    if (value < ADC_MAX) value = ADC_MAX;
    return (ADC_MIN - value) * 100 / (ADC_MIN - ADC_MAX);
}

/* Control LEDs based on conditions ----------------------------------*/
void ledControl(int humidity, int temperature) {
    if (humidity < set_moisture) PORTD |= (1 << WATERING_LED), PORTD &= ~(1 << DRYING_LED);
    else PORTD |= (1 << DRYING_LED), PORTD &= ~(1 << WATERING_LED);
    if (temperature < set_temp) PORTD |= (1 << HEATING_LED), PORTD &= ~(1 << COOLING_LED);
    else PORTD |= (1 << COOLING_LED), PORTD &= ~(1 << HEATING_LED);
}
