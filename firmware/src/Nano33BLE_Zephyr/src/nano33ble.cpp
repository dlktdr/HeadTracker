
#include "Nano33BLE.h"
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
#include "log.h"
#include "analog.h"
#include "joystick.h"

bool led_is_on = false;

TrackerSettings trkset;

void start(void)
{
   // Setup Serial
    serial_Init();

    // Setup Pins - io.cpp
    io_Init();

    // Start the BT Thread, Higher Prority than data. - bt.cpp
    bt_Init();

    // Actual Calculations - sense.cpp
    if(sense_Init()) {
        LOG_ERR("Unable to initalize sensors");
    }

    // Start SBUS - SBUS/uarte_sbus.cpp (Pins D0/TX, D1/RX)
    SBUS_Init(0,1);

    // PWM Outputs - Fixed to A0-A3
    PWM_Init(50); // Start PWM 50hz

    // USB Joystick
    joystick_init();

    // Load settings from flash - trackersettings.cpp
    trkset.loadFromEEPROM();


    // Main blinky light
	while (1) {
        digitalWrite(LED_BUILTIN,led_is_on);

        //serialWriteln("HT: MAIN");
		led_is_on = !led_is_on;
		k_msleep(100);
	}
}

#define PRIORITY_LOW 4
#define PRIORITY_MED 8
#define PRIORITY_HIGH 16
#define PRIORITY_RT 32

K_THREAD_DEFINE(io_Thread_id, 256, io_Thread, NULL, NULL, NULL, PRIORITY_LOW, 0, 0);
K_THREAD_DEFINE(serial_Thread_id, 8192, serial_Thread, NULL, NULL, NULL, PRIORITY_LOW, K_FP_REGS, 0);
K_THREAD_DEFINE(data_Thread_id, 2048, data_Thread, NULL, NULL, NULL, PRIORITY_LOW-2, K_FP_REGS, 0);
K_THREAD_DEFINE(bt_Thread_id, 4096, bt_Thread, NULL, NULL, NULL, PRIORITY_HIGH, 0, 0);
K_THREAD_DEFINE(sense_Thread_id, 4096, sense_Thread, NULL, NULL, NULL, PRIORITY_HIGH, K_FP_REGS, 2000);
K_THREAD_DEFINE(SBUS_Thread_id, 1024, SBUS_Thread, NULL, NULL, NULL, PRIORITY_RT, 0, 0);