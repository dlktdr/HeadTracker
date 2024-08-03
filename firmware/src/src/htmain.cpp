
#include "htmain.h"

#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

#include "PPMIn.h"
#include "PPMOut.h"
#include "analog.h"
#include "ble.h"
#include "io.h"
#include "joystick.h"
#include "btjoystick.h"

#include "pmw.h"
#include "sense.h"
#include "serial.h"
#include "soc_flash.h"
#include "trackersettings.h"
#include "uart_mode.h"

LOG_MODULE_REGISTER(htmain);

TrackerSettings trkset;

K_SEM_DEFINE(saveToFlash_sem, 0, 1);

// Wait for serial connection on the GUI port before starting.
//#define WAITFOR_DTR

void start(void)
{
  // Initalize IO
  LOG_INF("Starting IO");
  io_init();

  // Load settings from flash - trackersettings.cpp
  LOG_INF("Loading Settings");
  trkset.loadFromEEPROM();

  // Serial Setup, we have a CDC Device, enable USB
#if defined(DT_N_INST_0_zephyr_cdc_acm_uart)
  LOG_INF("USB starting");
  int ret = usb_enable(NULL);
  if (ret != 0) {
    LOG_ERR("USB unable to start");
    setLEDFlag(LED_HARDFAULT);
  }
#endif

  // USB Joystick
  LOG_INF("Joystick starting");
  joystick_init();

// Pause code here until connected via serial
#ifdef WAITFOR_DTR
  LOG_INF("PAUSING FOR SERIAL CONNECTION");
  const struct device *dev = DEVICE_DT_GET(DT_ALIAS(guiuart));
  uint32_t dtr = 0U;
  while (true) {
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    if (dtr) {
      break;
    } else {
      k_msleep(50);
    }
  }
#endif

  // Ininitialize GUI logging and GUI Serial
  LOG_INF("GUISerial starting");
  if(serial_init()) {
    LOG_ERR("GUISerial initialization failed");
    setLEDFlag(LED_HARDFAULT);
  }

  // Actual Calculations - sense.cpp
  LOG_INF("Sense starting");
  if(sense_Init()) {
    LOG_ERR("Sense initialization failed");
    setLEDFlag(LED_HARDFAULT);
  }

  // Start PPM Output
#if defined(HAS_PPMOUT)
  LOG_INF("PPMOut starting");
  if(PpmOut_init()) {
    LOG_WRN("PPMOut initialization failed");
  }
#else
  LOG_INF("PPMOut is not supported on this board");
#endif

  // Start PPM Input
#if defined(HAS_PPMIN)
  LOG_INF("PPMIn starting");
  if(PpmIn_init()) {
    LOG_WRN("PPMIn initalization failed");
  }
#else
  LOG_INF("PPMIn is not supported on this board");
#endif

  // Start External UART
#if defined(HAS_AUXSERIAL)
  LOG_INF("AuxUART starting");
  uart_init();
#else
  LOG_INF("AuxUART is not supported on this board");
#endif

  // PWM Outputs - Fixed to A0-A3
#if defined(HAS_PWMOUTPUTS)
  LOG_INF("PWM starting");
  PWM_Init(PWM_FREQUENCY);
#else
  LOG_INF("PWM is not supported on this board");
#endif

  // Start Bluetooth
#if defined(CONFIG_BT)
  LOG_INF("Bluetooth starting");
  // Check if center button is held down, force BT Configuration mode
  LOG_INF("Checking if center button is pressed");
  if (readCenterButton()) {
    LOG_INF("Button is pressed. Forcing bluetooth into configurator mode");
    trkset.setBtMode(BTPARAHEAD);
    setLEDFlag(LED_BTCONFIGURATOR);
  }
  bt_init();
#else
  LOG_INF("Bluetooth is not supported on this board");
#endif

  // Monitor if saving to EEPROM is required
  while (1) {
    if (!k_sem_take(&saveToFlash_sem, K_FOREVER)) {
      LOG_INF("Saving Settings");
      trkset.saveToEEPROM();
    }
  }
}

// Threads
K_THREAD_DEFINE(io_Thread_id, IO_STACK_SIZE, io_Thread, NULL, NULL, NULL, IO_THREAD_PRIO, K_FP_REGS, 0);
K_THREAD_DEFINE(serial_Thread_id, SERIAL_STACK_SIZE, serial_Thread, NULL, NULL, NULL, SERIAL_THREAD_PRIO, K_FP_REGS, 1000);
#if defined(CONFIG_BT)
K_THREAD_DEFINE(bt_Thread_id, BT_STACK_SIZE, bt_Thread, NULL, NULL, NULL, BT_THREAD_PRIO, 0, 0);
#endif
K_THREAD_DEFINE(sensor_Thread_id, SENSOR_STACK_SIZE, sensor_Thread, NULL, NULL, NULL, SENSOR_THREAD_PRIO, K_FP_REGS, 1500);
K_THREAD_DEFINE(calculate_Thread_id, CALCULATE_STACK_SIZE, calculate_Thread, NULL, NULL, NULL, CALCULATE_THREAD_PRIO, K_FP_REGS, 2000);
K_THREAD_DEFINE(uartTx_Thread_ID, UARTTX_STACK_SIZE, uartTx_Thread, NULL, NULL, NULL, UARTTX_THREAD_PRIO, 0, 1000);
K_THREAD_DEFINE(uartRx_Thread_ID, UARTRX_STACK_SIZE, uartRx_Thread, NULL, NULL, NULL, UARTRX_THREAD_PRIO, 0, 1000);
