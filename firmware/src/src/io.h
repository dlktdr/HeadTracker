#pragma once

#include <zephyr.h>
#include <drivers/gpio.h>

extern bool wasButtonPressed();
extern bool wasButtonLongPressed();
extern void pressButton();
extern void longPressButton();
extern void io_Thread();
extern void io_init();

extern volatile bool buttonpressed;

extern const device *gpios[2];

#define ANALOG_RESOLUTION 12

extern int dpintopin[];
extern int dpintoport[];

#define D_TO_PIN(x) (dpintopin[x])
#define D_TO_PORT(x) (dpintoport[x])
#define D_TO_32X_PIN(x) ((D_TO_PORT(x) * 32) + D_TO_PIN(x))

#define ARDUINO_D0_TX (32 * 1 + 3)                      /* P1.3 */
#define ARDUINO_D1_RX (32 * 1 + 10)                     /* P1.10 */
#define ARDUINO_D2 (32 * 1 + 11)                        /* P1.11 */
#define ARDUINO_D3 (32 * 1 + 12)                        /* P1.12 */
#define ARDUINO_D4 (32 * 1 + 15)                        /* P1.15 */
#define ARDUINO_D5 (32 * 1 + 13)                        /* P1.13 */
#define ARDUINO_D6 (32 * 1 + 14)                        /* P1.14 */
#define ARDUINO_D7 (32 * 0 + 23)                        /* P0.23 */

#define ARDUINO_D8 (32 * 0 + 21)                        /* P0.21 */
#define ARDUINO_D9 (32 * 0 + 27)                        /* P0.27 */
#define ARDUINO_D10 (32 * 1 + 2)                        /* P1.2 */
#define ARDUINO_D11_MOSI (32 * 1 + 1)                   /* P1.1 */
#define ARDUINO_D12_MISO (32 * 1 + 8)                   /* P1.8 */
#define ARDUINO_D13_SCK (32 * 0 + 13)                   /* P0.13 */

#define ARDUINO_A0 (32 * 0 + 4)                         /* P0.4 */
#define ARDUINO_A1 (32 * 0 + 5)                         /* P0.5 */
#define ARDUINO_A2 (32 * 0 + 30)                        /* P0.30 */
#define ARDUINO_A3 (32 * 0 + 29)                        /* P0.29 */
#define ARDUINO_A4_SDA (32 * 0 + 31)                    /* P0.31 */
#define ARDUINO_A5_SCL (32 * 0 + 2)                     /* P0.2 */
#define ARDUINO_A4 ARDUINO_A4_SDA
#define ARDUINO_A5 ARDUINO_A5_SCL
#define ARDUINO_A6 (32 * 0 + 28)                        /* P0.28 */
#define ARDUINO_A7 (32 * 0 + 3)                         /* P0.3 */

#define ARDUINO_LEDR (32 * 0 + 24)                      /* P0.24 */
#define ARDUINO_LEDG (32 * 0 + 16)                      /* P0.16 */
#define ARDUINO_LEDB (32 * 0 + 6)                       /* P0.6 */
#define ARDUINO_LEDPWR (32 * 1 + 9)                     /* P1.9 */

#define ARDUINO_INT_APDS (32 * 0 + 19)                  /* P0.19 */

#define ARDUINO_PDM_PWR (32 * 0 + 17)                   /* P0.17 */
#define ARDUINO_PDM_CLK (32 * 0 + 26)                   /* P0.26 */
#define ARDUINO_PDM_DIN (32 * 0 + 25)                   /* P0.25 */

#define ARDUINO_SDA1 (32 * 0 + 14)                      /* P0.14 */
#define ARDUINO_SCL1 (32 * 0 + 15)                      /* P0.15 */
#define ARDUINO_INTERNAL_I2C_PULLUP (32 * 1 + 0)        /* P1.0 */
#define ARDUINO_INTERNAL_VDD_ENV_ENABLE (32 * 0 + 22)   /* P0.22 */

#define LED_B ARDUINO_LEDB
#define LED_G ARDUINO_LEDG
#define LED_R ARDUINO_LEDR

#define HIGH 1
#define LOW 0

#define LEDR ARDUINO_LEDR
#define LEDG ARDUINO_LEDG
#define LEDB ARDUINO_LEDB
#define LED_BUILTIN ARDUINO_D13_SCK
#define INPUT_PULLUP (GPIO_INPUT|GPIO_PULL_UP)
#define pinMode(pin, mode) gpio_pin_configure(gpios[pin / 32], pin % 32, mode)
#define digitalWrite(pin, value) gpio_pin_set(gpios[pin / 32], pin % 32, value)
#define digitalRead(pin) gpio_pin_get(gpios[pin / 32], pin % 32)
#define HIGH 1
#define LOW 0
#define A0 ARDUINO_A0
#define A1 ARDUINO_A1
#define A2 ARDUINO_A2
#define A3 ARDUINO_A3
#define A4 ARDUINO_A4_SDA
#define A5 ARDUINO_A5_SCL
#define A6 ARDUINO_A6
#define A7 ARDUINO_A7

// Mapping Analogs to Arduino Pins
#define AN0 2
#define AN1 3
#define AN2 6
#define AN3 5
#define AN4 7
#define AN5 0
#define AN6 4
#define AN7 1

// LEDS
extern volatile uint32_t _ledmode;
#define LED_RUNNING 1<<0
#define LED_GYROCAL 1<<1
#define LED_BTCONNECTED 1<<2
#define LED_BTSCANNING 1<<3
#define LED_MAGCAL 1<<4

#define RGB_RED 0xFF<<16
#define RGB_GREEN 0xFF<<8
#define RGB_BLUE 0xFF

#define LED_MAX_SEQUENCE_COUNT 10

void setLEDFlag(uint32_t ledMode);
void clearLEDFlag(uint32_t ledMode);
void clearAllFlags();