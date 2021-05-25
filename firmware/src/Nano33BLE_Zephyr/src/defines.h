#pragma once

#define DATA_PERIOD 120
#define BT_PERIOD 20      // Bluetooth update rate
#define SERIAL_PERIOD 30  // Serial processing
#define IO_PERIOD 20      // milliseconds
#define SENSE_PERIOD 16666 // 100hz Update Rate
#define SBUS_PERIOD 20
#define UIRESPONSIVE_TIME 10000 // 10Seconds without an ack data will stop;

#define JSON_BUF_SIZE 3000
#define TX_RNGBUF_SIZE 1500
#define RX_RNGBUF_SIZE 1500

#define FW_VERSION "1.0"
#define FW_BOARD "NANO33BLE"

#define DEG_TO_RAD 0.017453295199
#define RAD_TO_DEG 57.29577951308

#define micros() k_cyc_to_us_floor32(k_cycle_get_32())
