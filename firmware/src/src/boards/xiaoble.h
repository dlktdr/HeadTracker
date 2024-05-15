#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board Features
#define HAS_AUXSERIAL
#define HAS_3DIODE_RGB
#define HAS_NOTIFYLED
#define HAS_CENTERBTN
#define HAS_PPMIN
#define HAS_PPMOUT
#define HAS_3CH_ANALOG
#define HAS_VOLTMON

// If this is the Sense board. Enable the IMU
#if defined(CONFIG_BOARD_XIAO_BLE_NRF52840_SENSE)
#define HAS_LSM6DS3
#endif

// Mapping Analog numbers to Analog pins
#define AN0 1 //
#define AN1 4 //
#define AN2 5 //
#define ANVOLTMON 7 // Battery Voltage
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
  PIN(CENTER_BTN,   NRFPIN(1, 13), "P1.13 D8") \
  PIN(PPMOUT,       NRFPIN(1, 15), "P1.15 D10") \
  PIN(PPMIN,        NRFPIN(1, 14), "P1.14 D9") \
  PIN(VOLTMON,      NRFPIN(0, 31), "P0.31 Analog Battery Voltage") \
  PIN(ANBATT_ENA,   NRFPIN(0, 14), "P0.14 Battery Monitor Enable") \
  PIN(AN0,          NRFPIN(0,  3), "P0.03 Analog 1 (AIN_1)") \
  PIN(AN1,          NRFPIN(0, 28), "P0.28 Analog 2 (AIN_4)") \
  PIN(AN2,          NRFPIN(0, 29), "P0.29 Analog 3 (AIN_5)") \
  PIN(TX,           NRFPIN(1, 11), "P1.11 TX")  \
  PIN(RX,           NRFPIN(1, 12), "P1.12 RX") \
  PIN(RXINVO,       NRFPIN(0, 10), "P0.10 UART RX, Out Inv") \
  PIN(RXINVI,       NRFPIN(0,  9), "P0.09 UART RX, Inp Inv") \
  PIN(LEDR,         NRFPIN(0, 26), "") \
  PIN(LEDG,         NRFPIN(0, 30), "") \
  PIN(LEDB,         NRFPIN(0,  6), "") \
  PIN(LED,          NRFPIN(0, 14), "") \
  PIN(TXINV,        NRFPIN(0, 19), "") \
  PIN(SCK,          NRFPIN(0, 21), "") \
  PIN(MOSI,         NRFPIN(0, 20), "") \
  PIN(MISO,         NRFPIN(0, 24), "") \
  PIN(FLSH_WP,      NRFPIN(0, 22), "") \
  PIN(FLSH_HLD,     NRFPIN(0, 23), "") \
  PIN(I2CSCL,       NRFPIN(0, 27), "") \
  PIN(I2CSDA,       NRFPIN(0,  7), "") \
  PIN(LSM6DS3PWR,   NRFPIN(1,  8), "P1.08 LSM6DS3 Power 6DPWR") \
  PIN(LSM6DS3INT,   NRFPIN(0, 11), "P0.11 LSM6DS3 Interrupt")

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

// TODO Find good values here
// Values below were determined by plotting Gyro Output (See sense.cpp, gyroCalibration())
#define GYRO_STABLE_DIFF 200.0f
#define ACC_STABLE_DIFF 2.5f