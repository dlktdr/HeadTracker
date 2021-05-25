#include <zephyr.h>
#include "trackersettings.h"
#include "io.h"

volatile bool buttonpressed=false;
volatile int butpin;



// Reset Button Pressed Flag on Read
bool wasButtonPressed()
{
  if(buttonpressed) {
    buttonpressed = false;
    return true;
  }
  return false;
}

void pressButton()
{
  buttonpressed = true;
}

// Any IO Related Tasks, e.g. button
void io_Thread()
{
    while(1) {
        // Make sure button pin is enabled
        if(butpin < 1 || butpin > 13 )
            return;

        // Check button inputs, set flag, could make this an ISR but button for sure will be down for at least 1ms, also debounces
        if(digitalRead(butpin) != 0)
            buttonpressed = true;

        k_msleep(IO_PERIOD);
    }
}

void io_Init()
{
    pinMode(ARDUINO_INTERNAL_VDD_ENV_ENABLE, GPIO_OUTPUT);
    pinMode(ARDUINO_INTERNAL_I2C_PULLUP, GPIO_OUTPUT);
    pinMode(LED_BUILTIN, GPIO_OUTPUT);
    pinMode(ARDUINO_LEDPWR, GPIO_OUTPUT);
    pinMode(LEDR, GPIO_OUTPUT);
    pinMode(LEDG, GPIO_OUTPUT);
    pinMode(LEDB, GPIO_OUTPUT);
    digitalWrite(ARDUINO_INTERNAL_VDD_ENV_ENABLE, HIGH);
    digitalWrite(ARDUINO_INTERNAL_I2C_PULLUP, HIGH);
    digitalWrite(ARDUINO_LEDPWR,HIGH);
    digitalWrite(LEDR,HIGH);
    digitalWrite(LEDG,HIGH);
    digitalWrite(LEDB,HIGH);

    // Pins used to check timing
    pinMode(ARDUINO_A0, GPIO_OUTPUT); // PWM 0
    pinMode(ARDUINO_A1, GPIO_OUTPUT); // PWM 1
    pinMode(ARDUINO_A2, GPIO_OUTPUT); // PWM 2
    pinMode(ARDUINO_A3, GPIO_OUTPUT); // PWM 3
    pinMode(ARDUINO_A4, GPIO_OUTPUT); // Timing Output
    pinMode(ARDUINO_A5, GPIO_OUTPUT); // Timing Output
    pinMode(ARDUINO_A6, GPIO_INPUT); // Analog input
    pinMode(ARDUINO_A7, GPIO_INPUT); // Analog input

    digitalWrite(ARDUINO_A4, HIGH);
    digitalWrite(ARDUINO_A5, HIGH);

    butpin = trkset.buttonPin();
}
