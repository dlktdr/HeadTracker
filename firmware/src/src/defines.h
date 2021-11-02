#pragma once

// Maj + Min + Rev Defined in Platformio.ini
#define VERSION FW_MAJ.CONCAT(FW_MIN,FW_REV)
#define FW_VERSION STRINGIFY(VERSION)
#define FW_BOARD "NANO33BLE"

// Button Down Time
#ifdef FLIGHT_STICK
#define BUTTON_HOLD_TIME 5000  // 5sec (Reset on hold for 5sec)
#else
#define BUTTON_HOLD_TIME 1
#endif
#define BUTTON_LONG_PRESS_TIME 1000  // 1 second, Enable/Disables Tilt/Roll/Pan

// Thread Periods
#define IO_PERIOD 50           // (ms) IO Period (button reading)
#define DATA_PERIOD 90          // (ms) GUI update rate
#define BT_PERIOD 12500         // (us) Bluetooth update rate
#define SERIAL_PERIOD 30        // (ms) Serial processing
#define SENSOR_PERIOD 16666     // (us) 60hz Read Sensors
#define CALCULATE_PERIOD 6666   // (us) 150hz IMU calculations
#define SBUS_PERIOD 20          // (ms) SBUS 50hz
#define PWM_FREQUENCY 50        // (ms) PWM Period
#define UIRESPONSIVE_TIME 10000 // (ms) 10Seconds without an ack data will stop;

// Analog Filters 1 Euro Filter
#define AN_CH_CNT 4
#define AN_FILT_FREQ 150
#define AN_FILT_MINCO 0.02
#define AN_FILT_SLOPE 6
#define AN_FILT_DERCO 1

// SBUS
#define SBUSIN_PIN 10 // RX Pin
#define SBUSIN_PORT 1
#define SBUSOUT_PIN 3
#define SBUSOUT_PORT 1

// Bluetooth
#define BT_MIN_CONN_INTER_MASTER 16  // When run as para master
#define BT_MAX_CONN_INTER_MASTER 16

#define BT_MIN_CONN_INTER_PERIF 8  // When run as para slave
#define BT_MAX_CONN_INTER_PERIF 8

#define BT_CONN_LOST_TIME 70 // 100 * 10ms = 0.7seconds

// Thread Priority Definitions
#define PRIORITY_LOW 12
#define PRIORITY_MED 9
#define PRIORITY_HIGH 6
#define PRIORITY_RT 3

// Thread Periods, Negative values mean cannot be pre-empted
#define IO_THREAD_PRIO PRIORITY_LOW
#define SERIAL_THREAD_PRIO PRIORITY_LOW
#define DATA_THREAD_PRIO PRIORITY_LOW
#define BT_THREAD_PRIO -15
#define SENSOR_THREAD_PRIO PRIORITY_MED
#define CALCULATE_THREAD_PRIO PRIORITY_HIGH
#define SBUS_THREAD_PRIO PRIORITY_MED + 1

// Perepherial Channels Used, Make sure no dupilcates here
// and can't be used by Zephyr
// Cannot use GPIOTE interrupt as I override the interrupt handler in PPMIN

// Known good 14,15,16,17,18,19
// Unusable 13
#define SBUSIN1_PPICH 16
#define SBUSIN2_PPICH 15
#define SBUSOUT_PPICH 14
#define PPMIN_PPICH1 17
#define PPMIN_PPICH2 18
#define PPMOUT_PPICH 19

#define SBUS_UARTE_CH 1

// GPIOTE
// Known good GPIOTE 0,1,2,3,4,5,6,7

#define SBUSOUT0_GPIOTE 4
#define SBUSOUT1_GPIOTE 5
#define SBUSIN0_GPIOTE 0
#define SBUSIN1_GPIOTE 1
#define SBUSIN2_GPIOTE 2
#define PPMIN_GPIOTE 6
#define PPMOUT_GPIOTE 7

#define SBUSIN_TIMER_CH 2
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
