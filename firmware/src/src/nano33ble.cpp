
#include "nano33ble.h"

#include <drivers/counter.h>
#include <nrfx_clock.h>

#include "PPMIn.h"
#include "PPMOut.h"
#include "analog.h"
#include "ble.h"
#include "io.h"
#include "joystick.h"
#include "log.h"
#include "pmw.h"
#include "sense.h"
#include "serial.h"
#include "soc_flash.h"
#include "trackersettings.h"
#include "uart_mode.h"

TrackerSettings trkset;

K_SEM_DEFINE(saveToFlash_sem, 0, 1);

void start(void)
{
  // Initalize IO
  io_init();

  // USB Joystick
  joystick_init();

  // Setup Serial
  logger_init();
  serial_init();

  // Actual Calculations - sense.cpp
  sense_Init();

  // Start Externam UART
  uart_init();

  // PWM Outputs - Fixed to A0-A3
#if defined(HAS_PWMOUTPUTS)
  PWM_Init(PWM_FREQUENCY);
#endif

  // Load settings from flash - trackersettings.cpp
  trkset.loadFromEEPROM();

  // Check if center button is held down, force BT Configuration mode
  if (readCenterButton()) {
    trkset.setBtMode(BTPARAHEAD);
    setLEDFlag(LED_BTCONFIGURATOR);
  }

  // Start the BT Thread
  bt_init();

  // Monitor if saving to EEPROM is required
  while (1) {
    if (!k_sem_take(&saveToFlash_sem, K_FOREVER)) {
      trkset.saveToEEPROM();
    }
  }
}

// Threads
K_THREAD_DEFINE(io_Thread_id, 512, io_Thread, NULL, NULL, NULL, IO_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(serial_Thread_id, 4096, serial_Thread, NULL, NULL, NULL, SERIAL_THREAD_PRIO,
                K_FP_REGS, 1000);
K_THREAD_DEFINE(bt_Thread_id, 1024, bt_Thread, NULL, NULL, NULL, BT_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(sensor_Thread_id, 1024, sensor_Thread, NULL, NULL, NULL, SENSOR_THREAD_PRIO,
                K_FP_REGS, 500);
K_THREAD_DEFINE(calculate_Thread_id, 1024, calculate_Thread, NULL, NULL, NULL,
                CALCULATE_THREAD_PRIO, K_FP_REGS, 1000);
K_THREAD_DEFINE(uartTx_Thread_ID, 1024, uartTx_Thread, NULL, NULL, NULL, UARTTX_THREAD_PRIO, 0,
                1000);
K_THREAD_DEFINE(uartRx_Thread_ID, 512, uartRx_Thread, NULL, NULL, NULL, UARTRX_THREAD_PRIO, 0,
                1000);
