
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
#include "log.h"
#include "pmw.h"
#include "sense.h"
#include "serial.h"
#include "soc_flash.h"
#include "trackersettings.h"
#include "uart_mode.h"

LOG_MODULE_REGISTER(main);

TrackerSettings trkset;

K_SEM_DEFINE(saveToFlash_sem, 0, 1);

// Wait for serial connection on the GUI port before starting.
//#define WAITFOR_DTR

void start(void)
{
  // Initalize IO
  LOG_INF("Starting IO");
  io_init();

  // Serial Setup, we have a CDC Device, enable USB
#if defined(DT_N_INST_0_zephyr_cdc_acm_uart)
  LOG_INF("Starting USB\n");
  int ret = usb_enable(NULL);
  if (ret != 0) {
    LOG_ERR("Unable to enable USB\n");
    setLEDFlag(LED_HARDFAULT);
  }
#endif

  // USB Joystick
  LOG_INF("Starting Joystick");
  joystick_init();

// Pause code here until connected via serial
#ifdef WAITFOR_DTR
  LOG_INF("Waiting for Serial Connection");
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
  LOG_INF("Starting GUI Serial");
  logger_init();
  if(serial_init()) {
    LOG_ERR("Serial_init failed");
    setLEDFlag(LED_HARDFAULT);
  }

  // Actual Calculations - sense.cpp
  LOG_INF("Starting Sense");
  if(sense_Init()) {
    LOG_ERR("Sense_Init failed");
    setLEDFlag(LED_HARDFAULT);
  }

  // Start PPM Output
  LOG_INF("Starting PPMOut");
  if(PpmOut_init()) {
    LOG_WRN("PPMOut_init failed");
  }

  // Start PPM Input
  LOG_INF("Starting PPMIn");
  if(PpmIn_init()) {
    LOG_WRN("PPMIn_init failed");
  }

  // Start External UART
  LOG_INF("Starting AuxUART");
  uart_init();

  // PWM Outputs - Fixed to A0-A3
#if defined(HAS_PWMOUTPUTS)
  LOG_INF("Starting PWM");
  PWM_Init(PWM_FREQUENCY);
#endif

  // Load settings from flash - trackersettings.cpp
  LOG_INF("Loading Settings");
  trkset.loadFromEEPROM();

  // Check if center button is held down, force BT Configuration mode
  LOG_INF("Checking Center Button");
  if (readCenterButton()) {
    LOG_INF("Center Button Held Down, Foring BT Configurator Mode");
    trkset.setBtMode(BTPARAHEAD);
    setLEDFlag(LED_BTCONFIGURATOR);
  }

  // Start the BT Thread
  #if defined(CONFIG_SOC_SERIES_NRF52X) // TODO** ESP work needed here, crash easy
  #if defined(CONFIG_BT)
  LOG_INF("Starting Bluetooth");
  bt_init();
  #endif
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
#if defined(CONFIG_SOC_SERIES_NRF52X) // TODO** ESP work needed here, crash easy
#if defined(CONFIG_BT)
K_THREAD_DEFINE(bt_Thread_id, BT_STACK_SIZE, bt_Thread, NULL, NULL, NULL, BT_THREAD_PRIO, 0, 0);
#endif
#endif
K_THREAD_DEFINE(sensor_Thread_id, SENSOR_STACK_SIZE, sensor_Thread, NULL, NULL, NULL, SENSOR_THREAD_PRIO, K_FP_REGS, 1500);
K_THREAD_DEFINE(calculate_Thread_id, CALCULATE_STACK_SIZE, calculate_Thread, NULL, NULL, NULL, CALCULATE_THREAD_PRIO, K_FP_REGS, 2000);
K_THREAD_DEFINE(uartTx_Thread_ID, UARTTX_STACK_SIZE, uartTx_Thread, NULL, NULL, NULL, UARTTX_THREAD_PRIO, 0, 1000);
K_THREAD_DEFINE(uartRx_Thread_ID, UARTRX_STACK_SIZE, uartRx_Thread, NULL, NULL, NULL, UARTRX_THREAD_PRIO, 0, 1000);
