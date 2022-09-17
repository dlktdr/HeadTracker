#include "io.h"

#include <zephyr.h>

#include "soc_flash.h"
#include "trackersettings.h"

volatile bool ioThreadRun = false;
const device *gpios[2];

volatile bool buttonpressed = false;
volatile bool longpressedbutton = false;
volatile int butpin;
volatile uint32_t _ledmode = 0;

// LED Blink Patterns
typedef struct {
  uint32_t RGB = 0;
  uint16_t time = 0;
} rgb_s;
rgb_s led_sequence[LED_MAX_SEQUENCE_COUNT];

//                  0 1  2  3  4  5  6  7  8  9  10 11 12 13
int dpintoport[] = {1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0};
int dpintopin[] = {3, 10, 11, 12, 15, 13, 14, 23, 21, 27, 2, 1, 8, 13};

// Reset Button Pressed Flag on Read
bool wasButtonPressed()
{
  if (buttonpressed) {
    buttonpressed = false;
    return true;
  }
  return false;
}

// Reset Button Pressed Flag on Read
bool wasButtonLongPressed()
{
  if (longpressedbutton) {
    longpressedbutton = false;
    return true;
  }
  return false;
}

// Reset Center
void pressButton() { buttonpressed = true; }

void longPressButton() { longpressedbutton = true; }

void setLEDFlag(uint32_t ledMode) { _ledmode |= ledMode; }

void clearLEDFlag(uint32_t ledMode) { _ledmode &= ~ledMode; }

void clearAllFlags() { _ledmode = 0; }

// Any IO Related Tasks, e.g. button, leds
void io_Thread()
{
  int pressedtime = 0;
  bool led_is_on = false;
  int led_on_time = 25;
  int led_off_time = 200;
  int rgb_sequence_no = 0;
  uint32_t rgb_timer = millis();
  uint32_t _counter = 0;

  while (1) {
    rt_sleep_ms(IO_PERIOD);
    if (!ioThreadRun || pauseForFlash) continue;

    // LEDS
    if (_ledmode & LED_GYROCAL) {
      led_on_time = 200;
      led_off_time = 25;
    } else if (_ledmode & LED_BTCONNECTED) {
      led_on_time = 800;
      led_off_time = 200;
    } else if (_ledmode & LED_MAGCAL) {
      led_on_time = 100;
      led_off_time = 25;
    } else {
      led_on_time = 25;
      led_off_time = 200;
    }

    if ((!led_is_on && (_counter % led_off_time == 0)) ||
        (led_is_on && (_counter % led_on_time == 0))) {
      led_is_on = !led_is_on;
      digitalWrite(LED_BUILTIN, led_is_on);
    }

    // Bluetooth Connected - Blue light on solid
    if (_ledmode & LED_BTCONNECTED) {
      led_sequence[0].RGB = RGB_BLUE;
      led_sequence[0].time = 100;
      led_sequence[1].time = 0;  // End Sequence

      // Bluetooth Scanning, Blue light slow blinking
    } else if (_ledmode & LED_BTSCANNING) {
      led_sequence[0].RGB = RGB_BLUE;
      led_sequence[0].time = 300;
      led_sequence[1].RGB = 0;
      led_sequence[1].time = 300;
      led_sequence[2].time = 0;  // End Sequence

      // Magnetometer Calibration Mode - Red, Blue, Pause
    } else if (_ledmode & LED_MAGCAL) {
      led_sequence[0].RGB = RGB_RED;
      led_sequence[0].time = 200;
      led_sequence[1].RGB = RGB_BLUE;
      led_sequence[1].time = 200;
      led_sequence[2].RGB = 0;  // Pause
      led_sequence[2].time = 200;
      led_sequence[3].time = 0;  // End Sequence

      // Gyro Calibration Mode - Long Red, Flashing
    } else if (_ledmode & LED_GYROCAL) {
      led_sequence[0].RGB = RGB_RED;
      led_sequence[0].time = 400;
      led_sequence[1].RGB = 0;
      led_sequence[1].time = 200;
      led_sequence[2].time = 0;  // End Sequence
    } else {
      rgb_sequence_no = 0;
      led_sequence[0].time = 0;
    }

    // Run the Sequence
    uint32_t curcolor = led_sequence[rgb_sequence_no].RGB;
    if (led_sequence[rgb_sequence_no].time == 0) curcolor = 0;

    // TODO - Replace me with PWM control
    if (curcolor & RGB_RED)
      digitalWrite(LEDR, LOW);
    else
      digitalWrite(LEDR, HIGH);
    if (curcolor & RGB_GREEN)
      digitalWrite(LEDG, LOW);
    else
      digitalWrite(LEDG, HIGH);
    if (curcolor & RGB_BLUE)
      digitalWrite(LEDB, LOW);
    else
      digitalWrite(LEDB, HIGH);

    if (millis() > rgb_timer + led_sequence[rgb_sequence_no].time) {
      if (rgb_sequence_no < LED_MAX_SEQUENCE_COUNT - 1 && led_sequence[rgb_sequence_no].time != 0)
        rgb_sequence_no++;
      else
        rgb_sequence_no = 0;
      rgb_timer = millis();  // Reset current time
    }

    _counter += IO_PERIOD;
    if (_counter > 10000) _counter = 0;

    static bool lastButtonDown = false;
    butpin = trkset.getButtonPin();

    // Make sure button pin is enabled
    if (butpin < 1 || butpin > 13) continue;

    pinMode(D_TO_32X_PIN(butpin), INPUT_PULLUP);
    bool buttonDown = digitalRead(D_TO_32X_PIN(butpin)) == 0;

    // Button pressed down
    if (buttonDown && !lastButtonDown) {
      pressedtime = 0;

      // Increment count if held down
    } else if (buttonDown && lastButtonDown) {
      pressedtime += IO_PERIOD;

      // Just Released
    } else if (!buttonDown && lastButtonDown) {
      if (pressedtime > BUTTON_LONG_PRESS_TIME) {
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
  if (ioThreadRun) return;
  gpios[0] = device_get_binding("GPIO_0");
  gpios[1] = device_get_binding("GPIO_1");

  pinMode(ARDUINO_INTERNAL_VDD_ENV_ENABLE, GPIO_OUTPUT);
  pinMode(ARDUINO_INTERNAL_I2C_PULLUP, GPIO_OUTPUT);
  digitalWrite(ARDUINO_INTERNAL_VDD_ENV_ENABLE, HIGH);
  digitalWrite(ARDUINO_INTERNAL_I2C_PULLUP, HIGH);

  // Pins used to check timing
  pinMode(ARDUINO_A0, GPIO_OUTPUT);  // PWM 0
  pinMode(ARDUINO_A1, GPIO_OUTPUT);  // PWM 1
  pinMode(ARDUINO_A2, GPIO_OUTPUT);  // PWM 2
  pinMode(ARDUINO_A3, GPIO_OUTPUT);  // PWM 3
  pinMode(ARDUINO_A4, GPIO_INPUT);   // Analog input
  pinMode(ARDUINO_A5, GPIO_INPUT);   // Analog input
  pinMode(ARDUINO_A6, GPIO_INPUT);   // Analog input
  pinMode(ARDUINO_A7, GPIO_INPUT);   // Analog input

  // Leds
  pinMode(LED_BUILTIN, GPIO_OUTPUT);
  pinMode(ARDUINO_LEDPWR, GPIO_OUTPUT);
  pinMode(LEDR, GPIO_OUTPUT);
  pinMode(LEDG, GPIO_OUTPUT);
  pinMode(LEDB, GPIO_OUTPUT);
  digitalWrite(ARDUINO_LEDPWR, HIGH);
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, HIGH);
  digitalWrite(LEDB, HIGH);

  ioThreadRun = true;
}
