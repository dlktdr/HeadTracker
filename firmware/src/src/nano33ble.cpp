
#include "nano33ble.h"

#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/counter.h>
#include <nrfx_clock.h>

#include "DCMAhrs/dcmahrs.h"
#include "PPMIn.h"
#include "PPMOut.h"
#include "uart_mode.h"
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

#define CLOCK_NODE DT_INST(0, nordic_nrf_clock)
static const struct device *clock0;

bool led_is_on = false;

TrackerSettings trkset;

void start(void)
{
  // Force High Accuracy Clock
  const char *clock_label = DT_LABEL(CLOCK_NODE);
  clock0 = device_get_binding(clock_label);
  if (clock0 == NULL) {
    printk("Failed to fetch clock %s\n", clock_label);
  }
  clock_control_on(clock0, CLOCK_CONTROL_NRF_SUBSYS_HF);

  // USB Joystick
  joystick_init();

  // Setup Serial
  logger_init();
  serial_init();

  // Actual Calculations - sense.cpp
  sense_Init();

  // Start the BT Thread
  bt_init();

  // Start Externam UART
  uart_init();

  // PWM Outputs - Fixed to A0-A3
#if defined(HAS_PWMOUTPUTS)
  PWM_Init(PWM_FREQUENCY);
#endif

  // Load settings from flash - trackersettings.cpp
  trkset.loadFromEEPROM();
}

#if defined(RTOS_ZEPHYR)
// Threads
K_THREAD_DEFINE(io_Thread_id, 512, io_Thread, NULL, NULL, NULL, IO_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(serial_Thread_id, 16384, serial_Thread, NULL, NULL, NULL, SERIAL_THREAD_PRIO,
                K_FP_REGS, 1000);
K_THREAD_DEFINE(bt_Thread_id, 4096, bt_Thread, NULL, NULL, NULL, BT_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(sensor_Thread_id, 4096, sensor_Thread, NULL, NULL, NULL, SENSOR_THREAD_PRIO,
                K_FP_REGS, 500);
K_THREAD_DEFINE(calculate_Thread_id, 4096, calculate_Thread, NULL, NULL, NULL,
                CALCULATE_THREAD_PRIO, K_FP_REGS, 1000);
K_THREAD_DEFINE(uartTx_Thread_ID, 1024, uartTx_Thread, NULL, NULL, NULL, UARTTX_THREAD_PRIO, 0, 1000);
K_THREAD_DEFINE(uartRx_Thread_ID, 1024, uartRx_Thread, NULL, NULL, NULL, UARTRX_THREAD_PRIO, 0, 1000);
K_THREAD_DEFINE(DCMnormalization_Thread_id, 1024, DCMnormalization_Thread, NULL, NULL, NULL,
                CALCULATE_THREAD_PRIO, K_FP_REGS, 3000);

#elif defined(RTOS_FREERTOS)
#error "TODO... Add tasks for FreeRTOS"
#else
#error "RTOS NOT SPECIFIED"
#endif