
#include "nano33ble.h"
#include "dataparser.h"
#include "sense.h"
#include "io.h"
#include "soc_flash.h"
#include "serial.h"
#include "bluetooth/ble.h"
#include "PPM/PPMOut.h"
#include "PPM/PPMIn.h"
#include "SBUS/uarte_sbus.h"
#include "PWM/pmw.h"
#include "Joystick/joystick.h"
#include "log.h"
#include "Analog/analog.h"
#include "Led/led.h"

#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <drivers/counter.h>
#include <nrfx_clock.h>

#define CLOCK_NODE DT_INST(0, nordic_nrf_clock)
static const struct device *clock0;

bool led_is_on = false;

TrackerSettings trkset;

void start(void)
{
    led_init();

    // Force High Accuracy Clock
    const char *clock_label = DT_LABEL(CLOCK_NODE);
	clock0 = device_get_binding(clock_label);
	if (clock0 == NULL) {
		printk("Failed to fetch clock %s\n", clock_label);
	}
    clock_control_on(clock0,CLOCK_CONTROL_NRF_SUBSYS_HF);

    // USB Joystick
    joystick_init();

    // Setup Serial
    serial_init();

    // Setup Pins - io.cpp
    io_init();

    // Start the BT Thread, Higher Prority than data. - bt.cpp
    bt_init();

    // Actual Calculations - sense.cpp
    if(sense_Init()) {
        LOG_ERR("Unable to initalize sensors");
    }

    // Start SBUS - SBUS/uarte_sbus.cpp (Pins D0/TX, D1/RX)
    sbus_init();

    // PWM Outputs - Fixed to A0-A3
    PWM_Init(PWM_FREQUENCY);

    // Load settings from flash - trackersettings.cpp
    trkset.loadFromEEPROM();

    // Do nothing in this thread
    while(1) {
        k_msleep(100000);
    }
}

// Threads
K_THREAD_DEFINE(io_Thread_id, 256, io_Thread, NULL, NULL, NULL, IO_THREAD_PRIO, 0, 1000);
K_THREAD_DEFINE(serial_Thread_id, 8192, serial_Thread, NULL, NULL, NULL, SERIAL_THREAD_PRIO, K_FP_REGS, 1000);
K_THREAD_DEFINE(data_Thread_id, 2048, data_Thread, NULL, NULL, NULL, DATA_THREAD_PRIO, K_FP_REGS, 1000);
K_THREAD_DEFINE(bt_Thread_id, 4096, bt_Thread, NULL, NULL, NULL, BT_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(sensor_Thread_id, 2048, sensor_Thread, NULL, NULL, NULL, SENSOR_THREAD_PRIO, K_FP_REGS, 1000);
K_THREAD_DEFINE(calculate_Thread_id, 4096, calculate_Thread, NULL, NULL, NULL, CALCULATE_THREAD_PRIO, K_FP_REGS, 1000);
K_THREAD_DEFINE(SBUS_Thread_id, 1024, sbus_Thread, NULL, NULL, NULL, SBUS_THREAD_PRIO, 0, 1000);
K_THREAD_DEFINE(led_Thread_id, 256, led_Thread, NULL, NULL, NULL, LED_THREAD_PRIO, 0, 0);
