#include <zephyr.h>
#include "trackersettings.h"
#include "io.h"

volatile bool ioThreadRun = false;
const device *gpios[2];

volatile bool buttonpressed=false;
volatile bool longpressedbutton=false;
volatile int butpin;
volatile uint32_t _ledmode = 0;

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

void setLEDFlag(uint32_t ledMode)
{
    _ledmode |= ledMode;
}

void clearLEDFlag(uint32_t ledMode)
{
    _ledmode &= ~ledMode;
}

void clearAllFlags(uint32_t ledMode)
{
    _ledmode = 0;
}

// Any IO Related Tasks, e.g. button, leds
void io_Thread()
{
    int pressedtime=0;
    bool led_is_on=false;
    int led_on_time=25;
    int led_off_time=200;
    uint32_t _counter=0;

    while(1) {
        rt_sleep_ms(IO_PERIOD);
        if(!ioThreadRun)
            continue;

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
            if(pressedtime > BUTTON_LONG_PRESS_TIME) {
                longPressButton();
            } else if (pressedtime > BUTTON_HOLD_TIME) {
                pressButton();
            }
        }
        lastButtonDown = buttonDown;

        // LEDS
        if(_ledmode & LED_GYROCAL) {
            led_on_time = 200;
            led_off_time = 25;
        } else if (_ledmode & LED_BTCONNECTED) {
            led_on_time = 800;
            led_off_time = 200;
        } else {
            led_on_time = 25;
            led_off_time = 200;
        }
        if((!led_is_on && (_counter % led_off_time == 0)) ||
            (led_is_on && (_counter % led_on_time == 0))) {
            led_is_on = !led_is_on;
            digitalWrite(LED_BUILTIN,led_is_on);
        }

        // RGB_LED Output
        if(_ledmode & LED_BTCONNECTED) // Blue LED = Bluetooth connected
            digitalWrite(LEDB,LOW);
        else
            digitalWrite(LEDB,HIGH);

        if(_ledmode & LED_GYROCAL) // Red LED Flashing = Gyro_in_calibration
            digitalWrite(LEDR,LOW);
        else
            digitalWrite(LEDR,HIGH);

        _counter+=IO_PERIOD;
        if(_counter > 10000)
            _counter = 0;
    }
}

void io_init()
{
    if(ioThreadRun)
        return;
    gpios[0] = device_get_binding("GPIO_0");
    gpios[1] = device_get_binding("GPIO_1");

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

    // Leds
    pinMode(LED_BUILTIN, GPIO_OUTPUT);
    pinMode(ARDUINO_LEDPWR, GPIO_OUTPUT);
    pinMode(LEDR, GPIO_OUTPUT);
    pinMode(LEDG, GPIO_OUTPUT);
    pinMode(LEDB, GPIO_OUTPUT);
    digitalWrite(ARDUINO_LEDPWR,HIGH);
    digitalWrite(LEDR,HIGH);
    digitalWrite(LEDG,HIGH);
    digitalWrite(LEDB,HIGH);

    ioThreadRun = true;
}

