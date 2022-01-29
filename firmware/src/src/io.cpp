#include <zephyr.h>
#include "trackersettings.h"
#include "io.h"

volatile bool buttonpressed=false;
volatile bool longpressedbutton=false;
volatile int butpin;

//                  0 1  2  3  4  5  6  7  8  9  10 11 12 13
int dpintoport[] = {1,1, 1 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,1 ,1 ,1 ,0 };
int dpintopin[]  = {3,10,11,12,15,13,14,23,21,27,2 ,1 ,8 ,13};

// Reset Button Pressed Flag on Read
bool wasButtonPressed()
{
    if(buttonpressed) {
        buttonpressed = false;
        return true;
    }
    return false;
}

// Reset Button Pressed Flag on Read
bool wasButtonLongPressed()
{
    if(longpressedbutton) {
        longpressedbutton = false;
        return true;
    }
    return false;
}

// Reset Center
void pressButton()
{
    buttonpressed = true;
}

void longPressButton()
{
    longpressedbutton = true;
}

// Any IO Related Tasks, e.g. button
void io_Thread()
{
    int pressedtime=0;
    while(1) {
        k_msleep(IO_PERIOD);

        static bool lastButtonDown=false;
        butpin = trkset.buttonPin();

        // Make sure button pin is enabled
        if(butpin < 1 || butpin > 13 )
            continue;

        bool buttonDown = digitalRead(D_TO_32X_PIN(butpin)) == 0;

        // Button pressed down
        if(buttonDown && !lastButtonDown) {
            pressedtime = 0;

        // Increment count if held down
        } else if (buttonDown && lastButtonDown) {
            pressedtime += IO_PERIOD;

        // Just Released
        } else if(!buttonDown && lastButtonDown) {
            if(pressedtime > BUTTON_LONG_PRESS_TIME && trkset.buttonPressMode()) {
                longPressButton();
            } else if (pressedtime > BUTTON_HOLD_TIME) {
                pressButton();
            }
        }
        lastButtonDown = buttonDown;
    }
}

void io_init()
{
    pinMode(ARDUINO_INTERNAL_VDD_ENV_ENABLE, GPIO_OUTPUT);
    pinMode(ARDUINO_INTERNAL_I2C_PULLUP, GPIO_OUTPUT);
    digitalWrite(ARDUINO_INTERNAL_VDD_ENV_ENABLE, HIGH);
    digitalWrite(ARDUINO_INTERNAL_I2C_PULLUP, HIGH);

    // Pins used to check timing
    pinMode(ARDUINO_A0, GPIO_OUTPUT); // PWM 0
    pinMode(ARDUINO_A1, GPIO_OUTPUT); // PWM 1
    pinMode(ARDUINO_A2, GPIO_OUTPUT); // PWM 2
    pinMode(ARDUINO_A3, GPIO_OUTPUT); // PWM 3
    pinMode(ARDUINO_A4, GPIO_INPUT); // Analog input
    pinMode(ARDUINO_A5, GPIO_INPUT); // Analog input
    pinMode(ARDUINO_A6, GPIO_INPUT); // Analog input
    pinMode(ARDUINO_A7, GPIO_INPUT); // Analog input
}
