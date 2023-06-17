#pragma once

// Defines specified at compile time. If not, use 0.0.0
#ifndef FW_VER_TAG
#define FW_VER_TAG 0.0
#endif
#ifndef FW_GIT_REV
#define FW_GIT_REV "-------"
#endif

// Include the correct board header file
#if defined(PCB_NANO33BLE)
#include "boards/nano33board.h"
#elif defined(PCB_NANO33BLE_SENSE2) // Most features are the same on sense rev2
#define PCB_NANO33BLE
#include "boards/nano33board.h"
#elif defined(PCB_DTQSYS)
#define FW_BOARD "DTQSYS"
#include "boards/dtqsys_ht.h"
#elif defined(PCB_XIAOSENSE)
#define FW_BOARD "XIAOSENSE"
#include "boards/xiaosense.h"
#else
#error "NO PCB DEFINED"
#endif

#if defined(DEBUG)
#define DEFAULT_LOG_LEVEL DEBUG
#else
#define DEFAULT_LOG_LEVEL INFO
#endif

#define BUTTON_HOLD_TIME 1           // How long should the button be held for a normal press (ms)
#define BUTTON_LONG_PRESS_TIME 1000  // How long to hold button Enable/Disables Tilt/Roll/Pan (ms)

// Thread Periods
#define IO_PERIOD 25           // (ms) IO Period (button reading)
#define BT_PERIOD 12500        // (us) Bluetooth update rate
#define SERIAL_PERIOD 30       // (ms) Serial processing
#define DATA_PERIOD 2          // Multiplier of Serial Period (Live Data Transmission Speed)
#define SENSOR_PERIOD 4000     // (us) Sensor Reads
#define CALCULATE_PERIOD 7000  // (us) Channel Calculations
#define UART_PERIOD 4000       // (us) Update rate of UART
#define PWM_FREQUENCY 50       // (ms) PWM Period
#define PAUSE_BEFORE_FLASH 60  // (ms) Time to pause all threads before Flash writing

// Analog Filters 1 Euro Filter
#define AN_CH_CNT 4
#define AN_FILT_FREQ 150
#define AN_FILT_MINCO 0.02
#define AN_FILT_SLOPE 6
#define AN_FILT_DERCO 1

// Bluetooth
#define BT_MIN_CONN_INTER_MASTER 10  // When run as para master
#define BT_MAX_CONN_INTER_MASTER 10

#define BT_MIN_CONN_INTER_PERIF 10  // When run as para slave
#define BT_MAX_CONN_INTER_PERIF 10

#define BT_CONN_LOST_TIME 100  // 100 * 10ms = 1seconds

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
#define UARTRX_THREAD_PRIO PRIORITY_LOW - 2
#define UARTTX_THREAD_PRIO PRIORITY_HIGH

// Buffer Sizes for Serial/JSON
#define JSON_BUF_SIZE 3000
#define TX_RNGBUF_SIZE 1500
#define RX_RNGBUF_SIZE 1500

// Math Defines
#define DEG_TO_RAD 0.017453295199
#define RAD_TO_DEG 57.29577951308

// Magnetometer, Initial Orientation, Samples to average
#define MADGSTART_SAMPLES 15
#define GYRO_STABLE_SAMPLES 400
#define GYRO_SAMPLE_WEIGHT 0.05
#define GYRO_FLASH_IF_OFFSET 0.5 // Save to flash if gyro is off more than 0.5 degrees/sec from flash value

// RTOS Specifics
#include "zephyr.h"
#define millis() k_cyc_to_ms_floor32(k_cycle_get_32())
#define millis64() k_uptime_get()
#define micros() k_cyc_to_us_floor32(k_cycle_get_32())
#define micros64() k_cyc_to_us_floor64(k_cycle_get_32())
#define rt_sleep_ms(x) k_msleep(x)
#define rt_sleep_us(x) k_usleep(x)