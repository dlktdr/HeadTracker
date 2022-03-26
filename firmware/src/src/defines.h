#pragma once

// Maj + Min + Rev Defined in Platformio.ini
#define VERSION FW_MAJ.CONCAT(FW_MIN,FW_REV)
#define FW_VERSION STRINGIFY(VERSION)
#define FW_BOARD "NANO33BLE"

#define BUTTON_HOLD_TIME 1 // How long should the button be held for a normal press (ms)
#define BUTTON_LONG_PRESS_TIME 1000  // How long to hold button Enable/Disables Tilt/Roll/Pan (ms)

// Thread Periods
#define IO_PERIOD 25            // (ms) IO Period (button reading)
#define BT_PERIOD 12500         // (us) Bluetooth update rate
#define SERIAL_PERIOD 30        // (ms) Serial processing
#define DATA_PERIOD 2           // Multiplier of Serial Period (Live Data Transmission Speed)
#define SENSOR_PERIOD 16666     // (us) 60hz Read Sensors
#define CALCULATE_PERIOD 6666   // (us) 150hz IMU calculations
#define PWM_FREQUENCY 50        // (ms) PWM Period
#define UIRESPONSIVE_TIME 10000 // (ms) 10Seconds without an ack data will stop;
#define RESET_PULSE_PERIOD 500000 // (us) pulse duration for the reset signal to TX

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
#define BT_MIN_CONN_INTER_MASTER 10  // When run as para master
#define BT_MAX_CONN_INTER_MASTER 10

#define BT_MIN_CONN_INTER_PERIF 10  // When run as para slave
#define BT_MAX_CONN_INTER_PERIF 10

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

// Threads initialized flags
extern volatile bool ioThreadRun;
extern volatile bool serialThreadRun;
extern volatile bool btThreadRun;
extern volatile bool senseTreadRun;
extern volatile bool sbusTreadRun;
extern volatile bool gyro_calibrated;

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

// Math Defines
#define DEG_TO_RAD 0.017453295199
#define RAD_TO_DEG 57.29577951308

// Gyro Calibration Defines
#define GYRO_STABLE_SAMPLES 100 // samples to average of not moving for a success gyro cal
#define GYRO_PASS_DIFF 4.0 // Differential less than this deg/sec^2 considered stable
#define GYRO_LP_BETA 0.9 // Gyro Sample Moving Average Beta (0.0-1

// Magnetometer, Initial Orientation, Samples to average
#define MADGSTART_SAMPLES 15

// RTOS Options


#if defined(RTOS_ZEPHYR)
#define micros() k_cyc_to_us_floor32(k_cycle_get_32())
#define micros64() k_cyc_to_us_floor64(k_uptime_get())
#define millis() k_cyc_to_ms_floor32(k_cycle_get_32())
#define rt_sleep_ms(x) k_msleep(x)
#define rt_sleep_us(x) k_usleep(x)

#elif defined(RTOS_FREERTOS)
#error ("FREE RTOS NOT IMPLEMENTED")
#define micros()
#define micros64()
#define millis()
#define rt_sleep_s(x)
#define rt_sleep_ms(x)
#define rt_sleep_us(x)
#else
#error ("NO RTOS DECLARED")
#endif