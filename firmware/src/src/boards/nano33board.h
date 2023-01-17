#pragma once

#include <stdint.h>

#include "boardsdefs.h"

// Pull up pin mode
#define INPUT_PULLUP (GPIO_INPUT | GPIO_PULL_UP)

// Board Features
#if defined(PCB_NANO33BLE_SENSE2)
  #define HAS_BMI270
  #define HAS_BMM150
#else
  #define HAS_LSM9DS1
#endif

#define HAS_APDS9960
#define HAS_3DIODE_RGB
#define HAS_POWERLED
#define HAS_NOTIFYLED
#define HAS_PWMOUTPUTS

// Mapping Analog numbers to Analog pins
#define AN0 7 // AN4 pin
#define AN1 0 // AN5 pin
#define AN2 4 // AN6 pin
#define AN3 1 // AN7 pin

// Pins (name, number, description)
// NOTE: These pins are an enum entry. e.g. IO_D2 = 0
//     - Use PIN_NAME_TO_NUM(IO_D2) to get actual the pin number
//     - The pin number can be converted back into the NRF port/pin
//       using functions PIN_TO_NRFPORT & PIN_TO_NRFPIN
//     - The string descrition for D2 would be StrPins[IO_D2]

#define PIN_X \
  PIN(D2,         NRFPIN(1, 11), "Gen Purpose IO") \
  PIN(D3,         NRFPIN(1, 12), "Gen Purpose IO" ) \
  PIN(D4,         NRFPIN(1, 15), "Gen Purpose IO" ) \
  PIN(D5,         NRFPIN(1, 13), "Gen Purpose IO" ) \
  PIN(D6,         NRFPIN(1, 14), "Gen Purpose IO" ) \
  PIN(D7,         NRFPIN(0, 23), "Gen Purpose IO" ) \
  PIN(D8,         NRFPIN(0, 21), "Gen Purpose IO" ) \
  PIN(D9,         NRFPIN(0, 27), "Gen Purpose IO" ) \
  PIN(D10,        NRFPIN(1,  2), "Gen Purpose IO") \
  PIN(D11,        NRFPIN(1,  1), "Gen Purpose IO") \
  PIN(D12,        NRFPIN(1,  8), "Gen Purpose IO") \
  PIN(LED,        NRFPIN(0, 13), "Notification LED") \
  PIN(PWR,        NRFPIN(1,  9), "Power LED") \
  PIN(LEDR,       NRFPIN(0, 24), "Reg LED") \
  PIN(LEDG,       NRFPIN(0, 16), "Green LED") \
  PIN(LEDB,       NRFPIN(0,  6), "Blue LED") \
  PIN(PWM0,       NRFPIN(0,  4), "PWM 0 Output (A0)") \
  PIN(PWM1,       NRFPIN(0,  5), "PWM 1 Output (A1)") \
  PIN(PWM2,       NRFPIN(0, 30), "PWM 2 Output (A2)") \
  PIN(PWM3,       NRFPIN(0, 29), "PWM 3 Output (A3)") \
  PIN(AN0,        NRFPIN(0, 31), "Analog 0 (A4)") \
  PIN(AN1,        NRFPIN(0,  2), "Analog 1 (A5)") \
  PIN(AN2,        NRFPIN(0, 28), "Analog 2 (A6)") \
  PIN(AN3,        NRFPIN(0,  3), "Analog 3 (A7)") \
  PIN(TX,         NRFPIN(1,  3), "UART Transmit")  \
  PIN(RX,         NRFPIN(1, 10), "UART Receive") \
  PIN(TXINV,      NRFPIN(1,  4), "UART TX, Inv Pin") \
  PIN(RXINVO,     NRFPIN(1, 13), "UART RX, Output Inv Pin") \
  PIN(RXINVI,     NRFPIN(1, 14), "UART RX, Input Inv Pin") \
  PIN(VDDENA,     NRFPIN(0, 22), "Internal Vdd Enable") \
  PIN(I2C_PU,     NRFPIN(1,  0), "Internal I2C Pull Up") \
  PIN(APDSINT,    NRFPIN(0, 19), "APDS Interrupt Pin") \
  PIN(BMI270INT1, NRFPIN(0, 11), "BMI270 Interrupt 1 Pin") \
  PIN(BMI270INT2, NRFPIN(0, 20), "BMI270 Interrupt 2 Pin") \
  END_IO_PINS \

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
#define pinMode(pin, mode) gpio_pin_configure(gpios[PIN_TO_NRFPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_NRFPIN(PIN_NAME_TO_NUM(pin)), mode)
#define digitalWrite(pin, value) gpio_pin_set(gpios[PIN_TO_NRFPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_NRFPIN(PIN_NAME_TO_NUM(pin)), value)
#define digitalRead(pin) gpio_pin_get(gpios[PIN_TO_NRFPORT(PIN_NAME_TO_NUM(pin))], PIN_TO_NRFPIN(PIN_NAME_TO_NUM(pin)))

//                        0   1   2  3   4   5   6   7   8   9  10 11 12  13
const int dpintoport[] = {1,  1,  1, 1,  1,  1,  1,  0,  0,  0,  1, 1, 1,  0};
const int dpintopin[] =  {3, 10, 11, 12, 15, 13, 14, 23, 21, 27, 2, 1, 8, 13};
const int dpintoenum[] = {0,  0, IO_D2, IO_D3, IO_D4, IO_D5, IO_D6, IO_D7, IO_D8, IO_D9, IO_D10, IO_D11, IO_D12};

#define D_TO_PIN(x) (dpintopin[x])
#define D_TO_PORT(x) (dpintoport[x])
#define D_TO_32X_PIN(x) ((D_TO_PORT(x) * 32) + D_TO_PIN(x))
#define D_TO_ENUM(x) (dpintoenum[x])

// Values below were determined by plotting Gyro Output (See sense.cpp, gyroCalibration())
#define GYRO_STABLE_DIFF 200.0f
#define ACC_STABLE_DIFF 2.5f
