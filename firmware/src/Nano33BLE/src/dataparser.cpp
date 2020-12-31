#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <chrono>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include "dataparser.h"
#include "main.h"
#include "serial.h"

using namespace rtos;
using namespace mbed;

// Buffer
static char tempdata[RT_BUF_SIZE];

// Serial input / output Buffers
CircularBuffer<char, RT_BUF_SIZE> serin;
CircularBuffer<char, TX_BUF_SIZE> serout;

// Booleans for if JSON has been received - ISR flags
volatile bool JSONready=false;
volatile bool JSONfault=false;
volatile bool SerBufOverflow=false;
volatile bool BuffToSmall=false;

void parseData(DynamicJsonDocument &json);

// Timers, Initially start timed out
//uint64_t dataSendTime =  Kernel::get_ms_count();
//uint64_t uiResponsive = Kernel::get_ms_count();
Kernel::Clock::time_point dataSendTime = Kernel::Clock::now() + std::chrono::milliseconds(DATA_SEND_TIME);
Kernel::Clock::time_point uiResponsive = Kernel::Clock::now();

bool sendData=false;

// Buffered write to serial
Mutex serWriteMutex;

// Serial RX Interrupt, Stay Fast Here
void serialrx_Int()
{  
  digitalWrite(LEDG,LOW); // Serial RX Green, ON

  // Read all available data from Serial  
  while(Serial.available()) {
    digitalWrite(LEDG,LOW);
    char sc = Serial.read();
    if(sc == 0x02) {  // Start Of Text Character, clear buffer
      serin.reset();        
      
    } else if (sc == 0x03) { // End of Text Characher, parse JSON data
      if(JSONready)   // Data already in buffer not yet read
        JSONfault = true;
      else  {
        // Check how much data is in the buffer
        int bsz = serin.size();
        if(bsz > RT_BUF_SIZE - 1) {
            BuffToSmall = true;
            serin.reset();
        }

        // Move from Circular Buffer into Character String
        char *dataptr = tempdata;
        for(int i=0; i < bsz; i++) 
            serin.pop(*(dataptr++));
        *dataptr = 0; // Null terminate

        // Notify user thread the character string is ready to read
        JSONready = true;
      }
      serin.reset();      
    }

    else { // Add data to buffer
      serin.push(sc);
      if(serin.full())
        SerBufOverflow = true;      
    }
  }      
  digitalWrite(LEDG,HIGH);  // Serial RX Green, OFF
}

// Handles all data transmission with the UI via Serial port

void data_Thread()
{     
  DynamicJsonDocument json(RT_BUF_SIZE);

  while(1) {    
    digitalWrite(LEDB,LOW); // Serial RX Blue, Off      
    // Don't like this but can't get bufferedserial or available bytes to write
    // functions to work, at least shouldn't block this way.
    int bytx = serout.size();
    char txa[60];
    bytx = MIN(bytx,60);
    for(int i =0;i<bytx;i++) {
      serout.pop(txa[i]);      
    }
    Serial.write(txa,bytx); // ONLY PLACE SERIAL.WRITE SHOULD BE USED!!

    // Check if data is ready        
    if(JSONready) {
      DeserializationError de = deserializeJson(json, tempdata);      
      if(de) {
        serialWrite("HT: DeserializeJson() Failed\r\n");
      }

      // ISR Flags
      __disable_irq();
      JSONready = false;
      __enable_irq();

      // Parse The JSON Data
      parseData(json);            
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

    if(dataSendTime < curtime && uiResponsive > curtime) {
      // Build JSON of Data
      json.clear();      
      dataMutex.lock();
      trkset.setJSONData(json);
      dataMutex.unlock();

      // Add the Command
      json["Cmd"] = "Data";               

      serialWriteJSON(json);
      
      // Reset Sent Timer          
      dataSendTime = Kernel::Clock::now() + std::chrono::milliseconds(DATA_SEND_TIME);
    } 

    digitalWrite(LEDB,HIGH); // Serial RX Blue, Off
    ThisThread::sleep_for(std::chrono::milliseconds(DATA_THREAD_PERIOD));
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

    // Store Settings
    } else if (strcmp(command, "Setttings") == 0) {
        serialWrite("HT: Saving Settings\r\n");           
        trkset.loadJSONSettings(json);
        trkset.saveToEEPROM();

    // Get settings
    } else if (strcmp(command, "GetSet") == 0) {  
        serialWrite("HT: Sending Settings\r\n");
        json.clear();
        trkset.setJSONSettings(json);
        json["Cmd"] = "Settings";
        serialWriteJSON(json);

    // ACK Received, Means the GUI is running, send it data
    } else if (strcmp(command, "ACK") == 0) {
        //serialWrite("HT: Ack Received\r\n");
        uiResponsive = Kernel::Clock::now() + std::chrono::milliseconds(UIRESPONSIVE_TIME);
        dataSendTime = Kernel::Clock::now() + std::chrono::milliseconds(DATA_SEND_TIME);        
    
    // Firmware Reqest
    } else if (strcmp(command, "FWR") == 0) {
      serialWrite("HT: FW Requested\r\n");
      json.clear();
      json["Cmd"] = "FW";
      json["Vers"] = FW_VERSION;
      json["Hard"] = FW_BOARD;
      serialWriteJSON(json);

    // Unknown Command
    } else {
        serialWrite("HT: Unknown Command\r\n");    
    }
}