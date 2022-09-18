#include "io.h"

#include <zephyr.h>

#include "soc_flash.h"
#include "trackersettings.h"

K_SEM_DEFINE(button_sem, 0, 1);
K_SEM_DEFINE(lngbutton_sem, 0, 1);

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

// Reset Button Pressed Flag on Read
bool wasButtonPressed()
{
  if (k_sem_take(&button_sem, K_NO_WAIT)) return false;
  return true;
}

// Reset Button Pressed Flag on Read
bool wasButtonLongPressed()
{
  if (k_sem_take(&lngbutton_sem, K_NO_WAIT)) return false;
  return true;
}

// Reset Center
void pressButton() { k_sem_give(&button_sem); }

void longPressButton() { k_sem_give(&lngbutton_sem); }

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
      digitalWrite(IO_LED, led_is_on);
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

#if defined(PCB_NANO33BLE)
    // TODO - Replace me with PWM control
    if (curcolor & RGB_RED)
      digitalWrite(IO_LEDR, 0);
    else
      digitalWrite(IO_LEDR, 1);
    if (curcolor & RGB_GREEN)
      digitalWrite(IO_LEDG, 0);
    else
      digitalWrite(IO_LEDG, 1);
    if (curcolor & RGB_BLUE)
      digitalWrite(IO_LEDB, 0);
    else
      digitalWrite(IO_LEDB, 1);
#elif defined(PCB_DTQSYS)
    // TODO.. WS2812 led here
#endif

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

#if defined(PCB_NANO33BLE)
    pinMode(D_TO_ENUM(butpin), INPUT_PULLUP);
    bool buttonDown = digitalRead(D_TO_ENUM(butpin)) == 0;
#else
    pinMode(IO_RESET_BTN, INPUT_PULLUP);
    bool buttonDown = digitalRead(IO_RESET_BTN) == 0;
#endif


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

#if defined(PCB_NANO33BLE)
  pinMode(IO_VDDENA, GPIO_OUTPUT);
  pinMode(IO_I2C_PU, GPIO_OUTPUT);
  pinMode(IO_LEDR, GPIO_OUTPUT);
  pinMode(IO_LEDG, GPIO_OUTPUT);
  pinMode(IO_LEDB, GPIO_OUTPUT);
  digitalWrite(IO_VDDENA, 1);
  digitalWrite(IO_I2C_PU, 1);
  digitalWrite(IO_LEDR, 1);
  digitalWrite(IO_LEDG, 1);
  digitalWrite(IO_LEDB, 1);
#endif

  // Pins used to check timing
  pinMode(IO_PWM0, GPIO_OUTPUT);  // PWM 0
  pinMode(IO_PWM1, GPIO_OUTPUT);  // PWM 1
  pinMode(IO_PWM2, GPIO_OUTPUT);  // PWM 2
  pinMode(IO_PWM3, GPIO_OUTPUT);  // PWM 3
  pinMode(IO_AN0, GPIO_INPUT);   // Analog input
  pinMode(IO_AN1, GPIO_INPUT);   // Analog input
  pinMode(IO_AN2, GPIO_INPUT);   // Analog input
  pinMode(IO_AN3, GPIO_INPUT);   // Analog input

  // Leds
  pinMode(IO_LED, GPIO_OUTPUT);
  pinMode(IO_PWR, GPIO_OUTPUT);
  digitalWrite(IO_PWR, 1);

  ioThreadRun = true;
}
