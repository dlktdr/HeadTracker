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

#include <stdio.h>
#include <device.h>
#include <drivers/uart.h>
#include <soc.h>
#include <drivers/uart/cdc_acm.h>
#include <drivers/usb/usb_dc.h>
#include <usb/class/usb_cdc.h>
#include <sys/ring_buffer.h>
#include <stdlib.h>
#include <zephyr.h>
#include <power/reboot.h>
#include <math.h>
#include "dataparser.h"
#include "serial.h"
#include "defines.h"
#include "ucrc16lib.h"
#include "dataparser.h"
#include "io.h"
#include "log.h"
#include "soc_flash.h"

// Wait for serial connection before starting..
//#define WAITFOR_DTR

void JSON_Process();
void serialrx_Process();
char* getJSONBuffer();
void parseData(DynamicJsonDocument &json);
uint16_t escapeCRC(uint16_t crc);
int buffersFilled();


// Ring Buffers
uint8_t ring_buffer_tx[TX_RNGBUF_SIZE]; // transmit buffer
uint8_t ring_buffer_rx[RX_RNGBUF_SIZE]; // receive buffer
uint8_t ring_buffer_json[RX_RNGBUF_SIZE]; // json data buffer
struct ring_buf ringbuf_tx;
struct ring_buf ringbuf_rx;
struct ring_buf ringbuf_json;

// JSON Data
char jsonbuffer[JSON_BUF_SIZE];
char *jsonbufptr = jsonbuffer;
DynamicJsonDocument json(JSON_BUF_SIZE);

// Flag that serial has been initalized
static bool serialStarted = false;

// Timers, Initially start timed out
volatile int64_t uiResponsive = k_uptime_get();

const struct device *dev;

static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

    // Force bootloader if baud set to 1200bps
    uint32_t baud=0;
    uart_line_ctrl_get(dev,UART_LINE_CTRL_BAUD_RATE, &baud);
    if(baud == 1200) {

    }

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			int recv_len, rb_len;
			uint8_t buffer[64];
			size_t len = MIN(ring_buf_space_get(&ringbuf_rx), sizeof(buffer));

			recv_len = uart_fifo_read(dev, buffer, len);
			rb_len = ring_buf_put(&ringbuf_rx, buffer, recv_len);
			if (rb_len < recv_len) {
                //LOG_ERR("RX Ring Buffer Full");
			}
		}
	}
}

void serial_Init()
{
	int ret;

	dev = device_get_binding("CDC_ACM_0");
	if (!dev) {
		return;
	}

	ret = usb_enable(NULL);
	if (ret != 0) {
		return;
	}

	ring_buf_init(&ringbuf_tx, sizeof(ring_buffer_tx), ring_buffer_tx);
    ring_buf_init(&ringbuf_rx, sizeof(ring_buffer_rx), ring_buffer_rx);
    ring_buf_init(&ringbuf_json, sizeof(ring_buffer_json), ring_buffer_json);

#ifdef WAITFOR_DTR
    uint32_t dtr = 0U;
	while (true) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		} else {
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
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

    serialStarted = true;
}


void serial_Thread()
{
    uint8_t buffer[64];

    while(1) {
        k_msleep(SERIAL_PERIOD);

        if(!serialStarted) {
            ring_buf_reset(&ringbuf_tx);
            continue;
        }

        digitalWrite(LEDG,LOW);

        // If serial not open, abort all transfers, clear buffer
        uint32_t dtr = 0;
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		if (!dtr) {
            ring_buf_reset(&ringbuf_tx);
            uart_tx_abort(dev);
            uiResponsive = k_uptime_get() - 1;


        // Port is open, send data
        } else {
            int rb_len = ring_buf_get(&ringbuf_tx, buffer, sizeof(buffer));
            if(rb_len) {
                int send_len = uart_fifo_fill(dev, buffer, rb_len);
                if (send_len < rb_len) {
                    LOG_ERR("USB CDC Ring Buffer Full, Dropped data");
                }
            }
        }

        serialrx_Process();

        digitalWrite(LEDG,HIGH);
    }
}

void serialrx_Process()
{
    char sc=0;

    // Get byte by byte data from the serial receive ring buffer
    while(ring_buf_get(&ringbuf_rx,(uint8_t*)&sc,1))
    {
         if(sc == 0x02) {  // Start Of Text Character, clear buffer
            jsonbufptr = jsonbuffer; // Reset Buffer

        } else if (sc == 0x03) { // End of Text Characher, parse JSON data
            *jsonbufptr = 0; // Null terminate
            JSON_Process();
            jsonbufptr = jsonbuffer; // Reset Buffer
        }
        else {
            // Check how much free data is in the buffer
            if(jsonbufptr >= jsonbuffer + sizeof(jsonbuffer) - 3) {
                serialWriteln("HT: Error JSON data too long, overflow");
                jsonbufptr = jsonbuffer; // Reset Buffer

            // Add data to buffer
            } else {
                *(jsonbufptr++) = sc;
            }
        }
    }
}

void JSON_Process()
{
    // CRC Check Data
    int len = strlen(jsonbuffer);
    if(len > 2) {
        uint16_t calccrc = escapeCRC(uCRC16Lib::calculate(jsonbuffer,len-sizeof(uint16_t)));
        if(calccrc != *(uint16_t*)(jsonbuffer+len-sizeof(uint16_t))) {
            serialWrite("\x15\r\n"); // Not-Acknowledged
            return;
        } else {
            serialWrite("\x06\r\n"); // Acknowledged
        }
        // Remove CRC from end of buffer
        jsonbuffer[len-sizeof(uint16_t)] = 0;
        DeserializationError de = deserializeJson(json, jsonbuffer);
        if(de) {
            if(de == DeserializationError::IncompleteInput)
                serialWrite("HT: DeserializeJson() Failed - Incomplete Input\r\n");
            else if(de == DeserializationError::InvalidInput)
                serialWrite("HT: DeserializeJson() Failed - Invalid Input\r\n");
            else if(de == DeserializationError::NoMemory)
                serialWrite("HT: DeserializeJson() Failed - NoMemory\r\n");
            else if(de == DeserializationError::EmptyInput)
                serialWrite("HT: DeserializeJson() Failed - Empty Input\r\n");
            else if(de == DeserializationError::TooDeep)
                serialWrite("HT: DeserializeJson() Failed - TooDeep\r\n");
            else
                serialWrite("HT: DeserializeJson() Failed - Other\r\n");
        } else {
            // Parse The JSON Data in dataparser.cpp
            parseData(json);
        }
    }
}

// New JSON data received from the PC
void parseData(DynamicJsonDocument &json)
{

    JsonVariant v = json["Cmd"];
    if(v.isNull()) {
      serialWrite("HT: Invalid JSON, No Command\r\n");
      return;
    }

    // For strcmp;
    const char *command = v;

    // Reset Center
    if(strcmp(command,"RstCnt") == 0) {
        serialWrite("HT: Resetting Center\r\n");
        pressButton();

    // Settings Sent from UI
    } else if (strcmp(command, "Set") == 0) {
        trkset.loadJSONSettings(json);
        serialWrite("HT: Storing Settings\r\n");

    // Save to Flash
    } else if (strcmp(command, "Flash") == 0) {
        serialWrite("HT: Saving to Flash\r\n");
        trkset.saveToEEPROM();

    // Erase
    } else if (strcmp(command, "Erase") == 0) {
        serialWrite("HT: Clearing Flash\r\n");
        socClearFlash();

    // Reboot
    } else if (strcmp(command, "Reboot") == 0) {
        sys_reboot(SYS_REBOOT_COLD);

    // Force Bootloader
    } else if (strcmp(command, "Boot") == 0) {
        (*((volatile uint32_t *) 0x20007FFCul)) = 0x07738135;
        NVIC_SystemReset();

    // Get settings
    } else if (strcmp(command, "Get") == 0) {
        serialWrite("HT: Sending Settings\r\n");
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
        serialWrite("HT: Clearing Data List\r\n");
        trkset.stopAllData();

    // Request Data Items
    } else if (strcmp(command, "RD") == 0) {
        serialWrite("HT: Data Added/Remove\r\n");
        // using C++11 syntax (preferred):
        JsonObject root = json.as<JsonObject>();
        for (JsonPair kv : root) {
            if(kv.key() == "Cmd")
                continue;
            trkset.setDataItemSend(kv.key().c_str(),kv.value().as<bool>());
        }

    // Firmware Reqest
    } else if (strcmp(command, "FW") == 0) {

        json.clear();
        json["Cmd"] = "FW";
        json["Vers"] = FW_VERSION;
        json["Hard"] = FW_BOARD;
        uiResponsive = k_uptime_get() - 1; // Don't send data on FW request.
        serialWriteJSON(json);
        serialWrite("HT: FW Requested\r\n");

    // Unknown Command
    } else {
        serialWrite("HT: Unknown Command\r\n");
        return;
    }

    // GUI responsive, update connected timer
    uiResponsive = k_uptime_get() + UIRESPONSIVE_TIME;
}

// Remove any of the escape characters
uint16_t escapeCRC(uint16_t crc)
{
    // Characters to escape out
    uint8_t crclow = crc & 0xFF;
    uint8_t crchigh = (crc >> 8) & 0xFF;
    if(crclow == 0x00 ||
       crclow == 0x02 ||
       crclow == 0x03 ||
       crclow == 0x06 ||
       crclow == 0x15)
        crclow ^= 0xFF; //?? why not..
    if(crchigh == 0x00 ||
       crchigh == 0x02 ||
       crchigh == 0x03 ||
       crchigh == 0x06 ||
       crchigh == 0x15)
        crchigh ^= 0xFF; //?? why not..
    return (uint16_t)crclow | ((uint16_t)crchigh << 8);
}

// Base write function.

void serialWrite(const char *data, int len)
{
    //  Lock to prevent two threads from adding in wrong order
    uint32_t key = irq_lock();
    if(ring_buf_space_get(&ringbuf_tx) < (uint32_t)len) { // Not enough room, drop it.
        irq_unlock(key);
        return;
    }
    int rb_len = ring_buf_put(&ringbuf_tx,(uint8_t *) data, len);
    irq_unlock(key);
    if(rb_len != len) {
        // Couldn't add to buffer for some reason
        __NOP();
    }
}

void serialWrite(std::string str)
{
    serialWrite(str.c_str());
}

void serialWrite(int val)
{
    char buf[50];
    __itoa(val,buf,10);
    // Append Output to the serial output buffer
    serialWrite(buf, strlen(buf));
}

void serialWrite(const char *data)
{
    // Append Output to the serial output buffer
    serialWrite(data, strlen(data));
}

void serialWrite(const char c) {
    char h = c;
    serialWrite(&h,1);
}

void serialWriteln(const char *data)
{
    // Make sure CRLF gets put
    uint32_t key = irq_lock();
    serialWrite(data, strlen(data));
    serialWrite("\r\n");
    irq_unlock(key);
}

/*int serialWriteF(const char *c, ...)
{
    char bff[256] = "Uknonen";
    int len=-1;

    va_list argptr;
    va_start(argptr, c);
    //len = vsnprintk(bff, sizeof(bff), c, argptr);
    va_end(argptr);
    serialWrite(bff,len);
    return len;
}*/

void serialWriteHex(const uint8_t *data, int len)
{
    for(int i=0; i < len; i++) {
        uint8_t nib1 = (*data >> 4) & 0x0F;
        uint8_t nib2 = *data++ & 0x0F;
        if(nib1 > 9)
            serialWrite((char)('A' + nib1-10));
        else
            serialWrite((char)('0' + nib1));
        if(nib2 > 9)
            serialWrite((char)('A' + nib2-10));
        else
            serialWrite((char)('0' + nib2));
        serialWrite(' ');
    }
}

// FIX Me to Not use as Much Stack.
void serialWriteJSON(DynamicJsonDocument &json)
{
    char data[TX_RNGBUF_SIZE];
    int br = serializeJson(json, data+1, TX_RNGBUF_SIZE-sizeof(uint16_t));
    uint16_t calccrc = escapeCRC(uCRC16Lib::calculate(data,br));

    if(br + 7 > TX_RNGBUF_SIZE)
        return;

    data[0] = 0x02;
    data[br+1] = (calccrc >> 8 ) & 0xFF;
    data[br+2] = calccrc & 0xFF;
    data[br+3] = 0x03;
    data[br+4] = '\r';
    data[br+5] = '\n';

    serialWrite(data, br+6);
}