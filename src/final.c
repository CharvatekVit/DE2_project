/***********************************************************************
 * 
 * The I2C (TWI) bus scanner tests all addresses and detects devices
 * that are connected to the SDA and SCL signals.
 * (c) 2023-2024 Tomas Fryza, MIT license
 *
 * Developed using PlatformIO and AVR 8-bit Toolchain 3.6.2.
 * Tested on Arduino Uno board and ATmega328P, 16 MHz.
 * 
 **********************************************************************/

/* Defines -----------------------------------------------------------*/


/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include <twi.h>            // I2C/TWI library for AVR-GCC
#include <uart.h>           // Peter Fleury's UART library
#include <stdlib.h>         // C library. Needed for number conversions
#include "timer.h"
#include <oled.h>           // OLED display library


/* Global variables --------------------------------------------------*/
// Declaration of "dht12" variable with structure "DHT_values_structure"
struct DHT_values_structure {
   uint8_t hum_int;
   uint8_t hum_dec;
   uint8_t temp_int;
   uint8_t temp_dec;
   uint8_t checksum;
} dht12;

// Flag for printing new data from sensor
volatile uint8_t new_sensor_data = 0;


// Slave and internal addresses of temperature/humidity sensor DHT12
#define SENSOR_ADR 0x5c
#define SENSOR_HUM_MEM 0
#define SENSOR_TEMP_MEM 2
#define SENSOR_CHECKSUM 4


/* Function definitions ----------------------------------------------*/
/**********************************************************************
* Function: Main function where the program execution begins
* Purpose:  Wait for new data from the sensor and sent them to UART.
* Returns:  none
**********************************************************************/
int main(void)
{
    char string[4];  // String for converting numbers by itoa()

    // TWI
    twi_init();

    // UART
    uart_init(UART_BAUD_SELECT(115200, F_CPU));

    // OLED
    oled_init(OLED_DISP_ON);
    oled_clrscr();

    sei();  // Enable global interrupts

    // Test if sensor is ready
    if (twi_test_address(SENSOR_ADR) == 0)
        uart_puts("I2C sensor detected\r\n");
    else {
        uart_puts("[ERROR] I2C device not detected\r\n");
        while (1);
    }

    // Display static text
    oled_gotoxy(0, 0);
    oled_puts("Humidity: ");
    oled_gotoxy(0, 5);
    oled_puts("Temperature: ");

    // Timer1
    TIM1_ovf_1sec();
    TIM1_ovf_enable();

    // Infinite loop
    while (1) {
        if (new_sensor_data == 1) {
            // Update humidity
            itoa(dht12.hum_int, string, 10);
            oled_gotoxy(0, 2);  // Adjust x position as needed
            oled_puts(string);
            oled_puts(".");
            itoa(dht12.hum_dec, string, 10);
            oled_puts(string);
            oled_puts(" %RH");

            // Update temperature
            itoa(dht12.temp_int, string, 10);
            oled_gotoxy(0, 7);  // Adjust x position as needed
            oled_puts(string);
            oled_puts(".");
            itoa(dht12.temp_dec, string, 10);
            oled_puts(string);
            oled_puts(" °C");

            // Copy buffer to display RAM
            oled_display();

            // Do not print it again and wait for the new data
            new_sensor_data = 0;
        }
    }

    // Will never reach this
    return 0;
}


/* Interrupt service routines ----------------------------------------*/
/**********************************************************************
* Function: Timer/Counter1 overflow interrupt
* Purpose:  Read temperature and humidity from DHT12, SLA = 0x5c.
**********************************************************************/
ISR(TIMER1_OVF_vect)
{
    // Read values from Temp/Humid sensor
    twi_start();
    if (twi_write((SENSOR_ADR<<1) | TWI_WRITE) == 0) {
        // Set internal memory location
        twi_write(SENSOR_HUM_MEM);
        twi_stop();

        // Read data from internal memory
        twi_start();
        twi_write((SENSOR_ADR<<1) | TWI_READ);
        dht12.hum_int = twi_read(TWI_ACK);
        dht12.hum_dec = twi_read(TWI_ACK);
        dht12.temp_int = twi_read(TWI_ACK);
        dht12.temp_dec = twi_read(TWI_ACK);
        dht12.checksum = twi_read(TWI_NACK);
        twi_stop();

        // Checksum validation
        uint8_t sum = dht12.hum_int + dht12.hum_dec + dht12.temp_int + dht12.temp_dec;
        if (sum == dht12.checksum) {
            new_sensor_data = 1;
        } else {
            uart_puts("[ERROR] Checksum mismatch\r\n");
        }
    } else {
        uart_puts("[ERROR] TWI communication failed\r\n");
        twi_stop();
    }
}