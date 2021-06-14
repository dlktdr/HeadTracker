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

#include <Arduino.h>
#include <mbed.h>

#include <chrono>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include "dataparser.h"
#include "main.h"
#include "serial.h"
#include "ucrc16lib.h"

using namespace rtos;
using namespace mbed;

// Timers, Initially start timed out
Kernel::Clock::time_point uiResponsive = Kernel::Clock::now();

bool sendData=false;
bool uiconnected=false;

uint16_t escapeCRC(uint16_t crc);

// Handles all data transmission with the UI via Serial port
void data_Thread()
{
    if(pauseThreads) {
        queue.call_in(std::chrono::milliseconds(100),data_Thread);
        return;
    }

    DynamicJsonDocument json(RX_BUF_SIZE);

    //while(1) {
    digitalWrite(LEDR,LOW); // Serial RX Blue, Off

    // Check if data is ready
    while(buffersFilled()) {
        char *buffer = getJSONBuffer();

        // CRC Check Data
         int len = strlen(buffer);
        if(len > 2) {
            uint16_t calccrc = escapeCRC(uCRC16Lib::calculate(buffer,len-sizeof(uint16_t)));
            if(calccrc != *(uint16_t*)(buffer+len-sizeof(uint16_t))) {
                serialWrite("\x15\r\n"); // Not-Acknowledged
                break;
            } else {
                serialWrite("\x06\r\n"); // Acknowledged
            }
            // Remove CRC from end of buffer
            buffer[len-sizeof(uint16_t)] = 0;

            DeserializationError de = deserializeJson(json, buffer);
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
                // Parse The JSON Data
                parseData(json);
            }
        }
    }

    if(JSONfault) {
        serialWrite("HT: JSON Data Lost\r\n");

        // ISR flags
        __disable_irq();
        JSONready = false;
        JSONfault = false;
        __enable_irq();
    }

    if(SerBufOverflow) {
      serialWrite("HT: Serial Buffer Overflow!\r\n");
        // ISR Flags
        __disable_irq();
        SerBufOverflow = false;
        JSONready = false;
        JSONfault = false;
        __enable_irq();
    }

    // Has it been enough time since last send of the data? Is the UI Still responsive?
    Kernel::Clock::time_point curtime = Kernel::Clock::now();

    if(uiResponsive > curtime) {
        uiconnected = true;
        // Build JSON of Data
        json.clear();
        dataMutex.lock();
        trkset.setJSONData(json);
        dataMutex.unlock();

        // Add the Command
        json["Cmd"] = "Data";

        serialWriteJSON(json);
    }  else
        uiconnected = false;


    digitalWrite(LEDR,HIGH); // Serial RX Blue, Off

    queue.call_in(std::chrono::milliseconds(DATA_PERIOD),data_Thread);
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
      serialWrite("HT: FW Requested\r\n");
      json.clear();
      json["Cmd"] = "FW";
      json["Vers"] = FW_VERSION;
      json["Hard"] = FW_BOARD;
      serialWriteJSON(json);

    // Unknown Command
    } else {
        serialWrite("HT: Unknown Command\r\n");
        return;
    }

    // Valid Command, update last gui connected time
    uiResponsive = Kernel::Clock::now() + std::chrono::milliseconds(UIRESPONSIVE_TIME);
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