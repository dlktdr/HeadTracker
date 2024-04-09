#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board Features
#if defined(BOARD_REV_SR2)
  #define HAS_BMI270
  #define HAS_BMM150
#else
  #define HAS_LSM9DS1
#endif

#define HAS_CENTERBTN
#define HAS_APDS9960
#define HAS_3DIODE_RGB
#define HAS_POWERLED
#define HAS_NOTIFYLED
#define HAS_PWMOUTPUTS
#define HAS_4CH_PWM
#define HAS_4CH_ANALOG
#define HAS_PPMOUT
#define HAS_PPMIN

// Mapping Analog numbers to Analog pins
#define AN0 7 // AN4 pin
#define AN1 0 // AN5 pin
#define AN2 4 // AN6 pin
#define AN3 1 // AN7 pin

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
  PIN(CENTER_BTN, NRFPIN(1, 12), "D3" ) \
  PIN(PPMIN,      NRFPIN(0, 27), "D9" ) \
  PIN(PPMOUT,     NRFPIN(1,  2), "D10") \
  PIN(PWM0,       NRFPIN(0,  4), "A0") \
  PIN(PWM1,       NRFPIN(0,  5), "A1") \
  PIN(PWM2,       NRFPIN(0, 30), "A2") \
  PIN(PWM3,       NRFPIN(0, 29), "A3") \
  PIN(AN0,        NRFPIN(0, 31), "A4") \
  PIN(AN1,        NRFPIN(0,  2), "A5") \
  PIN(AN2,        NRFPIN(0, 28), "A6") \
  PIN(AN3,        NRFPIN(0,  3), "A7") \
  PIN(TX,         NRFPIN(1,  3), "TX1 - AUXSERIAL")  \
  PIN(RX,         NRFPIN(1, 10), "RX0 - AUXSERIAL") \
  PIN(RXINVO,     NRFPIN(1, 13), "D5 - !SOLDER TO D6! - UARTRXInvert") \
  PIN(RXINVI,     NRFPIN(1, 14), "D6 - !SOLDER TO D5! - UARTRXInvert") \
  PIN(LED,        NRFPIN(0, 13), "") \
  PIN(PWR,        NRFPIN(1,  9), "") \
  PIN(LEDR,       NRFPIN(0, 24), "") \
  PIN(LEDG,       NRFPIN(0, 16), "") \
  PIN(LEDB,       NRFPIN(0,  6), "") \
  PIN(TXINV,      NRFPIN(1,  4), "") \
  PIN(VDDENA,     NRFPIN(0, 22), "") \
  PIN(I2C_PU,     NRFPIN(1,  0), "") \
  PIN(APDSINT,    NRFPIN(0, 19), "") \
  PIN(BMI270INT1, NRFPIN(0, 11), "") \
  PIN(BMI270INT2, NRFPIN(0, 20), "") \
  PIN(D2,         NRFPIN(1, 11), "") \
  PIN(D4,         NRFPIN(1, 15), "" ) \
  PIN(D7,         NRFPIN(0, 23), "" ) \
  PIN(D8,         NRFPIN(0, 21), "" ) \
  PIN(D11,        NRFPIN(1,  1), "") \
  PIN(D12,        NRFPIN(1,  8), "")

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
#define ACC_STABLE_DIFF 2.5f
