/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2021 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "serial.h"

#include <device.h>
#include <drivers/uart.h>
#include <drivers/uart/cdc_acm.h>
#include <drivers/usb/usb_dc.h>
#include <math.h>
#include <power/reboot.h>
#include <soc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ring_buffer.h>
#include <usb/class/usb_cdc.h>
#include <zephyr.h>

#include "io.h"
#include "log.h"
#include "nano33ble.h"
#include "soc_flash.h"
#include "trackersettings.h"
#include "ucrc16lib.h"


// Wait for serial connection before starting..
// #define WAITFOR_DTR

void serialrx_Process();
char *getJSONBuffer();
void parseData(DynamicJsonDocument &json);
uint16_t escapeCRC(uint16_t crc);
int buffersFilled();

// Connection state
uint32_t dtr = 0;

// Ring Buffers
uint8_t ring_buffer_tx[TX_RNGBUF_SIZE];  // transmit buffer
uint8_t ring_buffer_rx[RX_RNGBUF_SIZE];  // receive buffer
struct ring_buf ringbuf_tx;
struct ring_buf ringbuf_rx;
K_MUTEX_DEFINE(ring_tx_mutex);
K_MUTEX_DEFINE(ring_rx_mutex);

// JSON Data
char jsonbuffer[JSON_BUF_SIZE];
char *jsonbufptr = jsonbuffer;
DynamicJsonDocument json(JSON_BUF_SIZE);

// Mutex to protect Sense & Data Writes
K_MUTEX_DEFINE(data_mutex);

// Flag that serial has been initalized
static struct k_poll_signal serialThreadRunSignal =
    K_POLL_SIGNAL_INITIALIZER(serialThreadRunSignal);
struct k_poll_event serialRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &serialThreadRunSignal),
};

const struct device *dev;

static void interrupt_handler(const struct device *dev, void *user_data)
{
  ARG_UNUSED(user_data);

  while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
    if (uart_irq_rx_ready(dev)) {
      int recv_len, rb_len;
      uint8_t buffer[64];

      k_mutex_lock(&ring_rx_mutex, K_FOREVER);
      size_t len = MIN(ring_buf_space_get(&ringbuf_rx), sizeof(buffer));
      recv_len = uart_fifo_read(dev, buffer, len);
      rb_len = ring_buf_put(&ringbuf_rx, buffer, recv_len);
      k_mutex_unlock(&ring_rx_mutex);

      if (rb_len < recv_len) {
        // LOGE("RX Ring Buffer Full");
      }
    }
  }
}

void serial_init()
{
  int ret;

  dev = DEVICE_DT_GET(DT_N_INST_0_zephyr_cdc_acm_uart);
  if (!device_is_ready(dev)) {
		//LOG_ERR("CDC ACM device not ready");
		return;
  }

  ret = usb_enable(NULL);
  if (ret != 0) {
    return;
  }

  ring_buf_init(&ringbuf_tx, sizeof(ring_buffer_tx), ring_buffer_tx);
  k_mutex_init(&ring_tx_mutex);

  ring_buf_init(&ringbuf_rx, sizeof(ring_buffer_rx), ring_buffer_rx);
  k_mutex_init(&ring_rx_mutex);

#ifdef WAITFOR_DTR
  uint32_t dtr = 0U;
  while (true) {
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
    if (dtr) {
      break;
    } else {
      /* Give CPU resources to low priority threads. */
      rt_sleep_ms(100);
    }
  }
#endif

  /* They are optional, we use them to test the interrupt endpoint */
  ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
  ret = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);

  /* Wait 1 sec for the host to do all settings */
  k_busy_wait(1000000);

  uart_irq_callback_set(dev, interrupt_handler);

  /* Enable rx interrupts */
  uart_irq_rx_enable(dev);

  // Start Serial Thread
  k_poll_signal_raise(&serialThreadRunSignal, 1);
}

void serial_Thread()
{
  uint8_t buffer[64];
  static uint32_t datacounter = 0;

  while (1) {
    k_poll(serialRunEvents, 1, K_FOREVER);
    rt_sleep_ms(SERIAL_PERIOD);

    if (pauseForFlash) {
      continue;
    }

    // If serial not open, abort all transfers, clear buffer
    uint32_t new_dtr = 0;
    uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &new_dtr);

    k_mutex_lock(&ring_tx_mutex, K_FOREVER);

    // lost connection
    if (dtr && !new_dtr) {
      ring_buf_reset(&ringbuf_tx);
      uart_tx_abort(dev);
      trkset.stopAllData();
    }

    // gaining new connection
    if (!dtr && new_dtr) {
      ring_buf_reset(&ringbuf_tx);
      uart_tx_abort(dev);

      // Force bootloader if baud set to 1200bps TODO (Test Me)
      /*uint32_t baud=0;
      uart_line_ctrl_get(dev,UART_LINE_CTRL_BAUD_RATE, &baud);
      if(baud == 1200) {
        (*((volatile uint32_t *) 0x20007FFCul)) = 0x07738135;
        NVIC_SystemReset();
      }*/
    }

    // Port is open, send data
    if (new_dtr) {
      int rb_len = ring_buf_get(&ringbuf_tx, buffer, sizeof(buffer));
      if (rb_len) {
        int send_len = uart_fifo_fill(dev, buffer, rb_len);
        if (send_len < rb_len) {
          // LOG_ERR("USB CDC Ring Buffer Full, Dropped data");
        }
      }
    } else {
      ring_buf_reset(&ringbuf_tx);  // Clear buffer
    }
    k_mutex_unlock(&ring_tx_mutex);
    dtr = new_dtr;

    serialrx_Process();

    // Data output
    if (datacounter++ >= DATA_PERIOD) {
      datacounter = 0;

      // If sense thread is writing, wait until complete
      k_mutex_lock(&data_mutex, K_FOREVER);
      json.clear();
      trkset.setJSONData(json);
      if (json.size()) {
        json["Cmd"] = "Data";
        serialWriteJSON(json);
      }
      k_mutex_unlock(&data_mutex);
    }
  }
}

void serialrx_Process()
{
  char sc = 0;

  // Get byte by byte data from the serial receive ring buffer
  k_mutex_lock(&ring_rx_mutex, K_FOREVER);
  while (ring_buf_get(&ringbuf_rx, (uint8_t *)&sc, 1)) {
    if (sc == 0x02) {           // Start Of Text Character, clear buffer
      jsonbufptr = jsonbuffer;  // Reset Buffer

    } else if (sc == 0x03) {  // End of Text Characher, parse JSON data
      *jsonbufptr = 0;        // Null terminate
      JSON_Process(jsonbuffer);
      jsonbufptr = jsonbuffer;  // Reset Buffer
    } else {
      // Check how much free data is in the buffer
      if (jsonbufptr >= jsonbuffer + sizeof(jsonbuffer) - 3) {
        LOGE("Error JSON data too long, overflow");
        jsonbufptr = jsonbuffer;  // Reset Buffer

        // Add data to buffer
      } else {
        *(jsonbufptr++) = sc;
      }
    }
  }
  k_mutex_unlock(&ring_rx_mutex);
}

void JSON_Process(char *jsonbuf)
{
  // CRC Check Data
  int len = strlen(jsonbuf);
  if (len > 2) {
    k_mutex_lock(&ring_tx_mutex, K_FOREVER);
    uint16_t calccrc = escapeCRC(uCRC16Lib::calculate(jsonbuf, len - sizeof(uint16_t)));
    if (calccrc != *(uint16_t *)(jsonbuf + len - sizeof(uint16_t))) {
      serialWrite("\x15\r\n");  // Not-Acknowledged
      k_mutex_unlock(&ring_tx_mutex);
      return;
    } else {
      serialWrite("\x06\r\n");  // Acknowledged
    }
    // Remove CRC from end of buffer
    jsonbuf[len - sizeof(uint16_t)] = 0;

    k_mutex_lock(&data_mutex, K_FOREVER);
    DeserializationError de = deserializeJson(json, jsonbuf);
    if (de) {
      if (de == DeserializationError::IncompleteInput)
        LOGE("DeserializeJson() Failed - Incomplete Input");
      else if (de == DeserializationError::InvalidInput)
        LOGE("DeserializeJson() Failed - Invalid Input");
      else if (de == DeserializationError::NoMemory)
        LOGE("DeserializeJson() Failed - NoMemory");
      else if (de == DeserializationError::EmptyInput)
        LOGE("DeserializeJson() Failed - Empty Input");
      else if (de == DeserializationError::TooDeep)
        LOGE("DeserializeJson() Failed - TooDeep");
      else
        LOGE("DeserializeJson() Failed - Other");
    } else {
      // Parse The JSON Data in dataparser.cpp
      parseData(json);
    }
    k_mutex_unlock(&data_mutex);
    k_mutex_unlock(&ring_tx_mutex);
  }
}

// New JSON data received from the PC
void parseData(DynamicJsonDocument &json)
{
  JsonVariant v = json["Cmd"];
  if (v.isNull()) {
    LOGE("Invalid JSON, No Command");
    return;
  }

  // For strcmp;
  const char *command = v;

  // Reset Center
  if (strcmp(command, "RstCnt") == 0) {
    // TODO we should also log when the button on the device issues a Reset Center
    LOGI("Resetting Center");
    pressButton();

    // Settings Sent from UI
  } else if (strcmp(command, "Set") == 0) {
    trkset.loadJSONSettings(json);
    LOGI("Storing Settings");

    // Save to Flash
  } else if (strcmp(command, "Flash") == 0) {
    LOGI("Saving to Flash");
    k_sem_give(&saveToFlash_sem);

    // Erase
  } else if (strcmp(command, "Erase") == 0) {
    LOGI("Clearing Flash");
    socClearFlash();

    // Reboot
  } else if (strcmp(command, "Reboot") == 0) {
    sys_reboot(SYS_REBOOT_COLD);

    // Force Bootloader
  } else if (strcmp(command, "Boot") == 0) {
    (*((volatile uint32_t *)0x20007FFCul)) = 0x07738135;
    NVIC_SystemReset();

    // Get settings
  } else if (strcmp(command, "Get") == 0) {
    LOGI("Sending Settings");
    json.clear();
    trkset.setJSONSettings(json);
    json["Cmd"] = "Set";
    serialWriteJSON(json);

    // Im Here Received, Means the GUI is running
  } else if (strcmp(command, "IH") == 0) {
    __NOP();

    // Get a List of All Data Items
  } else if (strcmp(command, "DatLst") == 0) {
    json.clear();
    trkset.setJSONDataList(json);
    json["Cmd"] = "DataList";
    serialWriteJSON(json);

    // Stop All Data Items
  } else if (strcmp(command, "D--") == 0) {
    LOGI("Clearing Data List");
    trkset.stopAllData();

    // Request Data Items
  } else if (strcmp(command, "RD") == 0) {
    LOGI("Data Added/Remove");
    // using C++11 syntax (preferred):
    JsonObject root = json.as<JsonObject>();
    for (JsonPair kv : root) {
      if (kv.key() == "Cmd") continue;
      trkset.setDataItemSend(kv.key().c_str(), kv.value().as<bool>());
    }

    // Firmware Reqest
  } else if (strcmp(command, "FW") == 0) {
    DynamicJsonDocument fwjson(100);
    fwjson["Cmd"] = "FW";
    fwjson["Vers"] = STRINGIFY(FW_VER_TAG);
    fwjson["Hard"] = FW_BOARD;
    fwjson["Git"] = STRINGIFY(FW_GIT_REV);
    serialWriteJSON(fwjson);

    // Unknown Command
  } else {
    LOGW("Unknown Command");
    return;
  }
}

// Remove any of the escape characters
uint16_t escapeCRC(uint16_t crc)
{
  // Characters to escape out
  uint8_t crclow = crc & 0xFF;
  uint8_t crchigh = (crc >> 8) & 0xFF;
  if (crclow == 0x00 || crclow == 0x02 || crclow == 0x03 || crclow == 0x06 || crclow == 0x15)
    crclow ^= 0xFF;  //?? why not..
  if (crchigh == 0x00 || crchigh == 0x02 || crchigh == 0x03 || crchigh == 0x06 || crchigh == 0x15)
    crchigh ^= 0xFF;  //?? why not..
  return (uint16_t)crclow | ((uint16_t)crchigh << 8);
}

// Base write function.

void serialWrite(const char *data, int len)
{
  k_mutex_lock(&ring_tx_mutex, K_FOREVER);
  if (ring_buf_space_get(&ringbuf_tx) < (uint32_t)len) {  // Not enough room, drop it.
    k_mutex_unlock(&ring_tx_mutex);
    return;
  }
  int rb_len = ring_buf_put(&ringbuf_tx, (uint8_t *)data, len);
  k_mutex_unlock(&ring_tx_mutex);
  if (rb_len != len) {
    // TODO: deal with this case
    __NOP();
  }
}

void serialWrite(const char *data)
{
  // Append Output to the serial output buffer
  serialWrite(data, strlen(data));
}

int serialWriteF(const char *format, ...)
{
  va_list vArg;
  va_start(vArg, format);
  char buf[256];
  int len = vsnprintf(buf, sizeof(buf), format, vArg);
  va_end(vArg);
  serialWrite(buf, len);
  return len;
}

// FIX Me to Not use as Much Stack.
void serialWriteJSON(DynamicJsonDocument &json)
{
  char data[TX_RNGBUF_SIZE];

  k_mutex_lock(&ring_tx_mutex, K_FOREVER);
  int br = serializeJson(json, data + 1, TX_RNGBUF_SIZE - 7);
  uint16_t calccrc = escapeCRC(uCRC16Lib::calculate(data, br));

  if (br + 7 > TX_RNGBUF_SIZE) {
    k_mutex_unlock(&ring_tx_mutex);
    return;
  }

  data[0] = 0x02;
  data[br + 1] = (calccrc >> 8) & 0xFF;
  data[br + 2] = calccrc & 0xFF;
  data[br + 3] = 0x03;
  data[br + 4] = '\r';
  data[br + 5] = '\n';

  serialWrite(data, br + 6);
  k_mutex_unlock(&ring_tx_mutex);
}