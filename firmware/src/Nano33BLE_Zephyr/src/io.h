#pragma once

#include <zephyr.h>
#include "../include/arduino_nano_33_ble.h"

extern bool wasButtonPressed();
extern void pressButton();
extern void io_Thread();
extern void io_Init();

extern volatile bool buttonpressed;
extern struct arduino_gpio_t S_gpios;

#define ANALOG_RESOLUTION 12

extern int dpintopin[];
extern int dpintoport[];

// Make Arduino functions work in zypher
#define LEDR ARDUINO_LEDR
#define LEDG ARDUINO_LEDG
#define LEDB ARDUINO_LEDB
#define LED_BUILTIN ARDUINO_D13_SCK
#define INPUT_PULLUP (GPIO_INPUT|GPIO_PULL_UP)
#define pinMode(pin, mode) arduino_gpio_pinMode(&S_gpios, pin, mode)
#define digitalWrite(pin, state) arduino_gpio_digitalWrite(&S_gpios, pin, state)
#define digitalRead(pin) arduino_gpio_digitalRead(&S_gpios, pin)
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

