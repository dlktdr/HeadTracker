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

#include "dataparser.h"

#include "serial.h"
#include "main.h"
#include "ucrc16lib.h"
#include "dataparser.h"

// Buffer, Store up to RX_BUFFERS JSON Messages before loosing data.
char jsondatabuf[RX_BUFFERS][RX_BUF_SIZE];

// Serial input / output Buffers
CircularBuffer<char, RX_BUF_SIZE> serin;
CircularBuffer<char, RX_BUF_SIZE> serisr;
CircularBuffer<char, TX_BUF_SIZE> serout;

volatile bool JSONready;
volatile bool JSONfault;
volatile bool SerBufOverflow;
volatile bool BuffToSmall;
volatile int bufIndex=0;
volatile int bufsUsed=0;

void serial_Init()
{
    Serial.begin(115200); // Baud doesn't actually do anything with serial CDC
}

void serial_Thread()
{
    if(pauseThreads) {
        queue.call_in(std::chrono::milliseconds(100),serial_Thread);
        return;
    }

    digitalWrite(LEDG,LOW);
    uint32_t bytx = serout.size();
    char txa[SERIAL_TX_MAX_PACKET];
    bytx = MIN(bytx,SERIAL_TX_MAX_PACKET); // Packet size for USB FullSpeed is 64bytes..

    for(uint32_t i =0;i<bytx;i++) {
      serout.pop(txa[i]);
    }

    if(uiconnected) {
        uint32_t actualsent;
        Serial.send_nb((uint8_t *)txa,bytx,&actualsent);
        uint32_t bytesremaining = bytx-actualsent;
        if(bytesremaining != 0) {
            __NOP(); // *** Change to keep extra in buffer instead of drop?
        }
    }

    // Process Serial Data
    serialrx_Process();

    digitalWrite(LEDG,HIGH);
    queue.call_in(std::chrono::milliseconds(SERIAL_PERIOD),serial_Thread);
}

// Pop a JSON item off the buffer
char *getJSONBuffer()
{
    int bi = bufIndex;
    int bu = bufsUsed;
    bufsUsed--;

    int i = bi - bu;
    while (i >= RX_BUFFERS)
        i = i - RX_BUFFERS;
    while (i < 0)
        i = i + RX_BUFFERS;
    return jsondatabuf[i];
}

// Buffers Used
int buffersFilled()
{
    return bufsUsed;
}

// Serial RX Interrupt, Stay Fast as possible Here,
void serialrx_Int()
{
    digitalWrite(LEDG,LOW); // Serial RX Green, ON

    int bytes = Serial.available();
    for(int i = 0;i< bytes;i++) {
        serisr.push(Serial.read());
    }
    digitalWrite(LEDG,HIGH);  // Serial RX Green, OFF
}

void serialrx_Process()
{
    char sc=0;
    while(serisr.pop(sc))
    {
         if(sc == 0x02) {  // Start Of Text Character, clear buffer
            serin.reset();

        } else if (sc == 0x03) { // End of Text Characher, parse JSON data
            // All buffers filled? Ditch this message
            if(bufsUsed >= RX_BUFFERS-1)   {
                JSONfault = true;
                serin.reset();

            // Good to store to buffer
            } else  {
                // Move from the circular buffer into character String
                int bsz = serin.size();
                char *dataptr = jsondatabuf[bufIndex];
                for(int i=0; i < bsz; i++)
                    serin.pop(*(dataptr++));
                *dataptr = 0;

                bufIndex = (bufIndex+1) % RX_BUFFERS;
                bufsUsed++;
            }

            serin.reset();
        }

        else { // Add data to buffer
            // Check how much data is in the buffer

            if(serin.size() >= RX_BUF_SIZE - 1) {
                BuffToSmall = true;
                serin.reset();
            }
            //
            serin.push(sc);
            if(serin.full())
                SerBufOverflow = true;
        }
    }
}


void serialWrite(arduino::String str)
{
  serialWrite(str.c_str());
}

void serialWrite(float f)
{
  String str(f);
  serialWrite(str.c_str());
}

void serialWriteln(char const *data)
{
  int br = strlen(data);
  // Append Output to the serial output buffer

  for(int i =0; i < br; i++) {
    serout.push(data[i]);
  }
  serout.push('\r');
  serout.push('\n');
}

void serialWrite(int val)
{
  char buf[50];
  itoa(val,buf,10);
  int len = strlen(buf);
  // Append Output to the serial output buffer

  for(int i =0; i < len; i++) {
    serout.push(buf[i]);
  }
}

void serialWrite(char *data,int len) {

  // Append Output to the serial output buffer
  for(int i =0; i < len; i++) {
    serout.push(data[i]);
  }
}

void serialWrite(char const *data) {

  int br = strlen(data);
  // Append Output to the serial output buffer

  for(int i =0; i < br; i++) {
    serout.push(data[i]);
  }
}

void serialWrite(char c) {
  serout.push(c);
}

void serialWriteJSON(DynamicJsonDocument &json)
{
    char data[TX_BUF_SIZE];
    int br = serializeJson(json, data, TX_BUF_SIZE-sizeof(uint16_t));
    uint16_t calccrc = escapeCRC(uCRC16Lib::calculate(data,br));

    serout.push(0x02);
    // Append Output to the serial output buffer
    for(int i =0; i < br; i++) {
        serout.push(data[i]);
    }
    serout.push((calccrc >> 8 ) & 0xFF);
    serout.push(calccrc & 0xFF);
    serout.push(0x03);
    serout.push('\r');
    serout.push('\n');
}