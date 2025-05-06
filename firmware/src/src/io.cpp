#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

#include "io.h"
#include "defines.h"

#if defined(HAS_WS2812)
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/spi.h>
#endif

#if defined(CONFIG_SOC_SERIES_NRF52X)
#include <nrfx_gpiote.h>
#endif

#include "soc_flash.h"
#include "trackersettings.h"

void rstTimerFunc(struct k_timer *timer_id);
K_TIMER_DEFINE(rstCenterTimer, rstTimerFunc, NULL);

#if defined(CONFIG_SOC_SERIES_NRF52X)
// TODO: Find how to do this in new version of Zephyr
void setPinHighDrive(uint32_t pin) {

  if(PIN_TO_GPORT(PIN_NAME_TO_NUM(pin)) == 0) {
    NRF_P0->PIN_CNF[PIN_TO_GPIN(PIN_NAME_TO_NUM(pin))] = (NRF_P0->PIN_CNF[PIN_TO_GPIN(PIN_NAME_TO_NUM(pin))] & ~GPIO_PIN_CNF_DRIVE_Msk) |
          GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos;
  } else {
    NRF_P1->PIN_CNF[PIN_TO_GPIN(PIN_NAME_TO_NUM(pin))] = (NRF_P1->PIN_CNF[PIN_TO_GPIN(PIN_NAME_TO_NUM(pin))] & ~GPIO_PIN_CNF_DRIVE_Msk) |
          GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos;
  }
}
#endif

LOG_MODULE_REGISTER(io);

K_SEM_DEFINE(button_sem, 0, 1);
K_SEM_DEFINE(lngbutton_sem, 0, 1);

static struct k_poll_signal ioThreadRunSignal = K_POLL_SIGNAL_INITIALIZER(ioThreadRunSignal);
struct k_poll_event ioRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &ioThreadRunSignal),
};
const device *gpios[2];

volatile int butpin;
volatile uint32_t _ledmode = 0;

// LED Blink Patterns
typedef struct {
  uint32_t RGB = 0;
  uint16_t time = 0;
} rgb_s;
rgb_s led_sequence[LED_MAX_SEQUENCE_COUNT];

// Center button definded in the device tree?
#define CENTERBTN_NODE	DT_ALIAS(centerbtn)
static const struct gpio_dt_spec cbutton = GPIO_DT_SPEC_GET_OR(CENTERBTN_NODE, gpios, {0});

void io_init()
{
  gpios[0] = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#if defined(DT_N_NODELABEL_gpio1)
  gpios[1] = DEVICE_DT_GET(DT_NODELABEL(gpio1));
#endif

#if defined(CONFIG_BOARD_ARDUINO_NANO_33_BLE)
  pinMode(IO_VDDENA, GPIO_OUTPUT);
  pinMode(IO_I2C_PU, GPIO_OUTPUT);
  setPinHighDrive(IO_VDDENA);
  setPinHighDrive(IO_I2C_PU);
  digitalWrite(IO_I2C_PU, 1);
  // Hard Reset Sensor
  digitalWrite(IO_VDDENA, 0);
  k_msleep(200);
  digitalWrite(IO_VDDENA, 1);
#endif

#if defined(CONFIG_BOARD_XIAO_BLE_NRF52840_SENSE)
  pinMode(IO_LSM6DS3PWR, GPIO_OUTPUT);
  setPinHighDrive(IO_LSM6DS3PWR);
  // Hard Reset Sensor
  digitalWrite(IO_LSM6DS3PWR, 0);
  k_msleep(200);
  digitalWrite(IO_LSM6DS3PWR, 1);
#endif

#if defined(CONFIG_BOARD_XIAO_BLE)
  // Enable Battery Voltage Monitor
  pinMode(IO_ANBATT_ENA, GPIO_OUTPUT);
  digitalWrite(IO_ANBATT_ENA, 0);
#endif

#if defined(HAS_POWERHOLD)
  pinMode(IO_PWRHOLD, GPIO_OUTPUT);
  digitalWrite(IO_PWRHOLD, 1);
#endif

// Center Button defined in the device tree
#if DT_NODE_HAS_STATUS(CENTERBTN_NODE, okay)
	if (!gpio_is_ready_dt(&cbutton)) {
		LOG_ERR("Error: button device %s is not ready\n", cbutton.port->name);
	}
  int ret = gpio_pin_configure_dt(&cbutton, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n", ret, cbutton.port->name, cbutton.pin);
	}
#endif

#if defined(HAS_CENTERBTN)
  pinMode(IO_CENTER_BTN, INPUT_PULLUP);
#endif

#if defined(HAS_BUZZER)
  pinMode(IO_BUZZ, GPIO_OUTPUT);
  // Startup beep, beep
  digitalWrite(IO_BUZZ, 1);
  k_busy_wait(100000);
  digitalWrite(IO_BUZZ, 0);
  k_busy_wait(100000);
  digitalWrite(IO_BUZZ, 1);
  k_busy_wait(100000);
  digitalWrite(IO_BUZZ, 0);
#endif

#if defined(HAS_3DIODE_RGB)
  pinMode(IO_LEDR, GPIO_OUTPUT);
  pinMode(IO_LEDG, GPIO_OUTPUT);
  pinMode(IO_LEDB, GPIO_OUTPUT);
  digitalWrite(IO_LEDR, 1);
  digitalWrite(IO_LEDG, 1);
  digitalWrite(IO_LEDB, 1);
#endif

#if defined(HAS_POWERLED)
  pinMode(IO_PWR, GPIO_OUTPUT);
  digitalWrite(IO_PWR, 1);
#endif

#if defined(HAS_NOTIFYLED)
  pinMode(IO_LED, GPIO_OUTPUT);
#endif

#if defined(HAS_PWMOUTPUTS)
  pinMode(IO_PWM0, GPIO_OUTPUT);  // PWM 0
  pinMode(IO_PWM1, GPIO_OUTPUT);  // PWM 1
  pinMode(IO_PWM2, GPIO_OUTPUT);  // PWM 2
  pinMode(IO_PWM3, GPIO_OUTPUT);  // PWM 3
#endif

#if defined(AN0)
  pinMode(IO_AN0, GPIO_INPUT);
#endif
#if defined(AN1)
  pinMode(IO_AN1, GPIO_INPUT);
#endif
#if defined(AN2)
  pinMode(IO_AN2, GPIO_INPUT);
#endif
#if defined(AN3)
  pinMode(IO_AN3, GPIO_INPUT);
#endif

  setLEDFlag(LED_GYROCAL);

  k_poll_signal_raise(&ioThreadRunSignal, 1);
}

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

// Called after the reset center delay.
void rstTimerFunc(struct k_timer *timer_id) {
  k_sem_give(&button_sem);
}

// Call reset center after delay
void pressButton() {
  /* start a periodic timer that expires once every second */
  k_timer_start(&rstCenterTimer, K_MSEC(static_cast<uint16_t>(trkset.getRstDelay()*1000.0f)), K_NO_WAIT);
}

void longPressButton() { k_sem_give(&lngbutton_sem); }

void setLEDFlag(uint32_t ledMode) { _ledmode |= ledMode; }

void clearLEDFlag(uint32_t ledMode) { _ledmode &= ~ledMode; }

void clearAllLEDFlags() { _ledmode = 0; }

// Any IO Related Tasks, e.g. button, leds
void io_Thread()
{
#ifdef HAS_NOTIFYLED
  bool led_is_on = false;
  int led_on_time = 25;
  int led_off_time = 200;
#endif

  int pressedtime = 0;
  int rgb_sequence_no = 0;
  uint32_t rgb_timer = millis();
  uint32_t _counter = 0;

  while (1) {
    k_msleep(IO_PERIOD);
    k_poll(ioRunEvents, 1, K_FOREVER);

    if (k_sem_count_get(&flashWriteSemaphore) == 1) continue;

#if defined(HAS_NOTIFYLED)
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
#endif
    // Unrecoverable error, Solid Red
    if(_ledmode & LED_HARDFAULT) {
      led_sequence[0].RGB = RGB_RED;
      led_sequence[0].time = 10;
      led_sequence[1].time = 0;

    // Force BT head configuration mode
    } else if (_ledmode & LED_BTCONFIGURATOR) {
      led_sequence[0].RGB = RGB_RED;
      led_sequence[0].time = 300;
      led_sequence[1].RGB = RGB_GREEN;
      led_sequence[1].time = 300;
      led_sequence[2].RGB = RGB_BLUE;
      led_sequence[2].time = 300;
      led_sequence[3].time = 0;  // End Sequence

    // Gyro not calibrated but bluetooth is connected. Mostly Blue, Pulse of Red
    } else if (_ledmode & LED_GYROCAL && _ledmode & LED_BTCONNECTED) {
      led_sequence[0].RGB = RGB_BLUE;
      led_sequence[0].time = 500;
      led_sequence[1].RGB = RGB_RED;
      led_sequence[1].time = 20;
      led_sequence[2].time = 0;  // End Sequence

    // Gyro not calibrated, bluetooth is searching. Red Pulsing, Pulse of Blue
    } else if (_ledmode & LED_GYROCAL && _ledmode & LED_BTSCANNING) {
      led_sequence[0].RGB = RGB_BLUE;
      led_sequence[0].time = 50;
      led_sequence[1].RGB = RGB_RED;
      led_sequence[1].time = 400;
      led_sequence[2].RGB = RGB_OFF;
      led_sequence[2].time = 350;
      led_sequence[3].time = 0;  // End Sequence

    // Gyro Calibration Mode - Red slow, equal flashing
    } else if (_ledmode & LED_GYROCAL) { // Priority 2
      led_sequence[0].RGB = RGB_RED;
      led_sequence[0].time = 400;
      led_sequence[1].RGB = RGB_OFF;
      led_sequence[1].time = 400;
      led_sequence[2].time = 0;  // End Sequence

    // Bluetooth Connected - Blue light on solid
    } else if (_ledmode & LED_BTCONNECTED) {
      led_sequence[0].RGB = RGB_BLUE;
      led_sequence[0].time = 100;
      led_sequence[1].time = 0;  // End Sequence

    // Bluetooth Scanning, Blue light slow blinking
    } else if (_ledmode & LED_BTSCANNING) {
      led_sequence[0].RGB = RGB_BLUE;
      led_sequence[0].time = 300;
      led_sequence[1].RGB = RGB_OFF;
      led_sequence[1].time = 300;
      led_sequence[2].time = 0;  // End Sequence

      // Magnetometer Calibration Mode - Red, Blue, Off
    } else if (_ledmode & LED_MAGCAL) {
      led_sequence[0].RGB = RGB_RED;
      led_sequence[0].time = 200;
      led_sequence[1].RGB = RGB_BLUE;
      led_sequence[1].time = 200;
      led_sequence[2].RGB = RGB_OFF;
      led_sequence[2].time = 200;
      led_sequence[3].time = 0;  // End Sequence

    } else {
      rgb_sequence_no = 0;
      led_sequence[0].RGB = 0;
      led_sequence[0].time = 0;
    }

    // Run the Sequence
    uint32_t curcolor = led_sequence[rgb_sequence_no].RGB;
    if (led_sequence[rgb_sequence_no].time == 0 && rgb_sequence_no > 0) {
      curcolor = led_sequence[rgb_sequence_no-1].RGB;
    }

#if defined(HAS_3DIODE_RGB)
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
#endif

#if defined(HAS_WS2812)
    const struct device *strip = DEVICE_DT_GET(DT_NODELABEL(led_strip));
	  if (strip) {
      struct led_rgb pixel;
      pixel.b = curcolor & 0xFF;
      pixel.g = (curcolor >> 8) & 0xFF;
      pixel.r = (curcolor >> 16) & 0xFF;
      led_strip_update_rgb(strip, &pixel, 1);
    }
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
    bool buttonDown = readCenterButton();

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

bool readCenterButton()
{
#if defined(HAS_CENTERBTN)
    pinMode(IO_CENTER_BTN, INPUT_PULLUP);
    return digitalRead(IO_CENTER_BTN) == 0;
#endif
}
