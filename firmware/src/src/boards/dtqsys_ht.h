#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board features
#define HAS_WS2812
#define HAS_BUZZER
#define HAS_NOTIFYLED
#define HAS_MPU6500
#define HAS_QMC5883
#define HAS_AUXSERIAL
#define HAS_CENTERBTN
#define HAS_PPMOUT
#define HAS_PPMIN
#define HAS_3CH_ANALOG
#define HAS_VOLTMON

#define AN0 5 // Pin 0.29
#define AN1 0 // Pin 0.02
#define AN2 4 // Pin 0.28
#define ANVOLTMON 1 // Battery Voltage
#define ANVOLTMON_SCALE 2.0f
#define ANVOLTMON_OFFSET 0.0f

/*  Pins (name, number, description)
   NOTE: These pins are an enum entry. e.g. IO_CENTER_BTN = 0
       - Use PIN_NAME_TO_NUM(IO_D2) to get actual the pin number
       - The pin number can be converted back into the NRF port/pin
         using functions PIN_TO_GPORT & PIN_TO_GPIN
       - The string descrition for CENTER_BTN would be StrPinDescriptions[IO_CENTER_BTN]
       - The string of the pin name would be StrPins[CENTER_BTN]

   Change the pins to whatever you wish here. Some pins might be defined in the
   board's devicetree overlay file (e.g. UART). You will have to change them there
   & should make sure they match here so the GUI can show the correct pinout.

   Leave descriptions empty for pins if you don't want it to
   show up in the pinout on the GUI
   */

#define PIN_X \
  PIN(CENTER_BTN,   NRFPIN(1, 13), "") \
  PIN(VOLTMON,      NRFPIN(0,  3), "") \
  PIN(AN0,          NRFPIN(0, 29), "Analog 0 (AIN_5)") \
  PIN(AN1,          NRFPIN(0,  2), "Analog 1 (AIN_0)") \
  PIN(AN2,          NRFPIN(0, 28), "Analog 2 (AIN_4)") \
  PIN(LEDWS,        NRFPIN(1, 10), "") \
  PIN(LED,          NRFPIN(0, 13), "") \
  PIN(PPMOUT,       NRFPIN(0, 31), "") \
  PIN(PPMIN,        NRFPIN(0, 30), "") \
  PIN(BUZZ,         NRFPIN(1, 11), "") \
  PIN(TX,           NRFPIN(0,  9), "")  \
  PIN(RX,           NRFPIN(0, 10), "") \
  PIN(TXINV,        NRFPIN(0,  4), "") \
  PIN(RXINVO,       NRFPIN(0,  0), "") \
  PIN(RXINVI,       NRFPIN(0,  1), "") \
  PIN(I2CSDA,       NRFPIN(0,  5), "") \
  PIN(I2CSCL,       NRFPIN(1,  9), "")

typedef enum {
#define PIN(NAME, PINNO, DESC) IO_##NAME,
  PIN_X
#undef PIN
} pins_e;

const int8_t PinNumber[] = {
#define PIN(NAME, PINNO, DESC) PINNO,
    PIN_X
#undef PIN
};

// Required pin setting functions
#define pinMode(pin, mode) gpio_pin_configure(gpios[PIN_TO_GPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_GPIN(PIN_NAME_TO_NUM(pin)), mode)
#define digitalWrite(pin, value) gpio_pin_set(gpios[PIN_TO_GPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_GPIN(PIN_NAME_TO_NUM(pin)), value)
#define digitalRead(pin) gpio_pin_get(gpios[PIN_TO_GPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_GPIN(PIN_NAME_TO_NUM(pin)))

// Values below were determined by plotting Gyro Output (See sense.cpp, gyroCalibration())
#define GYRO_STABLE_DIFF 200.0f
#define ACC_STABLE_DIFF 3.5f
