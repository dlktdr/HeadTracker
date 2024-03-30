#pragma once

// Defines specified at compile time. If not, use 0.0.0
#ifndef FW_VER_TAG
#define FW_VER_TAG 0.0
#endif
#ifndef FW_GIT_REV
#define FW_GIT_REV "-------"
#endif

#ifndef BOARD_REV
#define BOARD_REV 0
#endif

//#pragma message ("Board is " CONFIG_BOARD)

// The majority of features are the same on Sense2

#if defined(CONFIG_BOARD_ARDUINO_NANO_33_BLE)
#define FW_BOARD "NANO33BLE"
#include "boards/nano33board.h"
#define ARDUINO_BOOTLOADER
#elif defined(CONFIG_BOARD_DTQSYS_HT)
#define FW_BOARD "DTQSYS"
#include "boards/dtqsys_ht.h"
#define ARDUINO_BOOTLOADER
#elif defined(CONFIG_BOARD_XIAO_BLE_SENSE) || defined(CONFIG_BOARD_XIAO_BLE)
#define FW_BOARD "XIAOSENSE"
#include "boards/xiaosense.h"
#define SEEED_BOOTLOADER
#elif defined(CONFIG_SOC_ESP32C3)
#define FW_BOARD "ESP32C3"
#include "boards/esp32c3.h"
#elif defined(CONFIG_BOARD_M5STICKC_PLUS)
#define FW_BOARD "M5CSTICK_PLUS"
#include "boards/m5stickc_plus.h"
#else
#error NO COMPATIBLE BOARD DEFINED
#endif

#if defined(DEBUG)
#define DEFAULT_LOG_LEVEL DEBUG
#else
#define DEFAULT_LOG_LEVEL INFO
#endif

#define BUTTON_HOLD_TIME 1           // How long should the button be held for a normal press (ms)
#define BUTTON_LONG_PRESS_TIME 1000  // How long to hold button Enable/Disables Tilt/Roll/Pan (ms)

// Thread Periods
#if defined(CONFIG_SOC_SERIES_NRF52X)
#define IO_PERIOD 25           // (ms) IO Period (button reading)
#define BT_PERIOD 12500        // (us) Bluetooth update rate
#define SERIAL_PERIOD 30       // (ms) Serial processing
#define DATA_PERIOD 2          // Multiplier of Serial Period (Live Data Transmission Speed)
#define SENSOR_PERIOD 4000     // (us) Sensor Reads
#define CALCULATE_PERIOD 7000  // (us) Channel Calculations
#define UART_PERIOD 4000       // (us) Update rate of UART
#define PWM_FREQUENCY 50       // (ms) PWM Period
#define PAUSE_BEFORE_FLASH 60  // (ms) Time to pause all threads before Flash writing
#else
#define IO_PERIOD 25           // (ms) IO Period (button reading)
#define BT_PERIOD 12500        // (us) Bluetooth update rate
#define SERIAL_PERIOD 30       // (ms) Serial processing
#define DATA_PERIOD 2          // Multiplier of Serial Period (Live Data Transmission Speed)
#define SENSOR_PERIOD 12000     // (us) Sensor Reads
#define CALCULATE_PERIOD 12000  // (us) Channel Calculations
#define UART_PERIOD 4000       // (us) Update rate of UART
#define PWM_FREQUENCY 50       // (ms) PWM Period
#define PAUSE_BEFORE_FLASH 60  // (ms) Time to pause all threads before Flash writing
#endif

// Thread Stack Sizes
#if defined(CONFIG_SOC_SERIES_NRF52X)
#define IO_STACK_SIZE 512
#define SERIAL_STACK_SIZE 4096
#define BT_STACK_SIZE 1024
#define SENSOR_STACK_SIZE 1024
#define CALCULATE_STACK_SIZE 1024
#define UARTTX_STACK_SIZE 1024
#define UARTRX_STACK_SIZE 512
#else
#define IO_STACK_SIZE 512
#define SERIAL_STACK_SIZE 4096
#define BT_STACK_SIZE 1024
#define SENSOR_STACK_SIZE 2048
#define CALCULATE_STACK_SIZE 2048
#define UARTTX_STACK_SIZE 1024
#define UARTRX_STACK_SIZE 512
#endif

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

// Perepherial Channels Used, Make sure no dupilcates here
// and can't be used by Zephyr
// Cannot use GPIOTE interrupt as I override the interrupt handler in PPMIN

#define PPMOUT_PPICH 0
#define SERIALIN1_PPICH 1
#define SERIALIN2_PPICH 2
#define SERIALOUT_PPICH 3
#define PPMIN_PPICH1 4
#define PPMIN_PPICH2 5
// 6 Used
// 7 Enable always gets flipped off
// 8+ ??

#define SERIAL_UARTE_CH 1

// GPIOTE
// Known good GPIOTE 0,1,2,3,4,5,6,7
#define SERIALOUT0_GPIOTE 4
#define SERIALOUT1_GPIOTE 5
#define SERIALIN0_GPIOTE 0
#define SERIALIN1_GPIOTE 1
#define SERIALIN2_GPIOTE 2
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

// Math Defines
#define DEG_TO_RAD 0.017453295199f
#define RAD_TO_DEG 57.29577951308f

// Magnetometer, Initial Orientation, Samples to average
#define MADGSTART_SAMPLES 15
#define GYRO_STABLE_SAMPLES 400
#define GYRO_SAMPLE_WEIGHT 0.05f
#define GYRO_FLASH_IF_OFFSET 0.5f // Save to flash if gyro is off more than 0.5 degrees/sec from flash value

// Time macros
#include "zephyr/kernel.h"
#define millis() k_cyc_to_ms_floor32(k_cycle_get_32())
#define millis64() k_uptime_get()
#define micros() k_cyc_to_us_floor32(k_cycle_get_32())
#define micros64() k_cyc_to_us_floor64(k_cycle_get_32())
