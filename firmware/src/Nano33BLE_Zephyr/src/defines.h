#pragma once

// Version
#define FW_VERSION "2.0"
#define FW_BOARD "NANO33BLE"

// Thread Periods
#define IO_PERIOD 20            // (ms) IO Period (button reading)
#define DATA_PERIOD 100         // (ms) GUI update rate
#define BT_PERIOD 18            // (ms) Bluetooth update rate 50hz
#define SERIAL_PERIOD 30        // (ms) Serial processing
#define SENSOR_PERIOD 20000     // (us) 50hz Read Sensors
#define CALCULATE_PERIOD 6666   // (us) 150hz IMU calculations
#define SBUS_PERIOD 20          // (ms) SBUS 50hz
#define UIRESPONSIVE_TIME 10000 // (ms) 10Seconds without an ack data will stop;

// Thread Priorities
#define PRIORITY_LOW 4
#define PRIORITY_MED 8
#define PRIORITY_HIGH 16
#define PRIORITY_RT 32

// Perepherial Channels Used, Make sure no dupilcates here
// and can't be used by Zephyr
// Cannot use GPIOTE interrupt as I override the interrupt handler in PPMIN

#define SBUSOUT_PPICH 16
#define PPMIN_PPICH1 17
#define PPMIN_PPICH2 18
#define PPMOUT_PPICH 19

#define SBUSOUT_UARTE_CH 1

#define SBUS_GPIOTE1 4 // Input GPIOTE
#define SBUS_GPIOTE2 5 // Output GPIOTE (Inverted)
#define PPMIN_GPIOTE 6
#define PPMOUT_GPIOTE 7

#define PPMOUT_TIMER_CH 3
#define PPMIN_TIMER_CH 4

#define PPMIN_TMRCOMP_CH 0
#define PPMOUT_TMRCOMP_CH 0

// Buffer Sizes for Serial/JSON
#define JSON_BUF_SIZE 3000
#define TX_RNGBUF_SIZE 1500
#define RX_RNGBUF_SIZE 1500

#define DEG_TO_RAD 0.017453295199
#define RAD_TO_DEG 57.29577951308

#define micros() k_cyc_to_us_floor32(k_cycle_get_32())
