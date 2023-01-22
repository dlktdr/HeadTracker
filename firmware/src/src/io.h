#pragma once

#include <drivers/gpio.h>
#include <zephyr.h>


extern bool wasButtonPressed();
extern bool wasButtonLongPressed();
extern void pressButton();
extern void longPressButton();
extern void io_Thread();
extern void io_init();

extern volatile bool buttonpressed;

extern const device *gpios[2];

#define ANALOG_RESOLUTION 12

// LEDS
extern volatile uint32_t _ledmode;
#define LED_RUNNING (1 << 0)
#define LED_GYROCAL (1 << 1)
#define LED_BTCONNECTED (1 << 2)
#define LED_BTSCANNING (1 << 3)
#define LED_MAGCAL (1 << 4)
#define LED_BTCONFIGURATOR (1 << 5)

#define RGB_OFF 0
#define RGB_RED (0xFF << 16)
#define RGB_GREEN (0xFF << 8)
#define RGB_BLUE 0xFF

#define LED_MAX_SEQUENCE_COUNT 5

void setLEDFlag(uint32_t ledMode);
void clearLEDFlag(uint32_t ledMode);
void clearAllLEDFlags();
bool readCenterButton();