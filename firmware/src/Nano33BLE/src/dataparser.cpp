#include <Arduino.h>
#include <mbed.h>

#include <chrono>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include "dataparser.h"
#include "main.h"
#include "serial.h"

using namespace rtos;
using namespace mbed;

// Timers, Initially start timed out
Kernel::Clock::time_point uiResponsive = Kernel::Clock::now();

bool sendData=false;

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
    while(buffersFilled() > 0) {
        char *buffer = getJSONBuffer();
        DeserializationError de = deserializeJson(json, buffer);
        if(de) {
            if(de == DeserializationError::IncompleteInput)
                serialWrite("HT: DeserializeJson() Failed - IncompleteInput\r\n");
            else if(de == DeserializationError::InvalidInput)
                serialWrite("HT: DeserializeJson() Failed - InvalidInput\r\n");
            else if(de == DeserializationError::NoMemory)
                serialWrite("HT: DeserializeJson() Failed - NoMemory\r\n");
            else if(de == DeserializationError::NotSupported)
                serialWrite("HT: DeserializeJson() Failed - NotSupported\r\n");
            else if(de == DeserializationError::TooDeep)
                serialWrite("HT: DeserializeJson() Failed - TooDeep\r\n");
            else 
                serialWrite("HT: DeserializeJson() Failed - Other\r\n");      
        }
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

    if(uiResponsive > curtime) {
        // Build JSON of Data
        json.clear();      
        dataMutex.lock();
        trkset.setJSONData(json);
        dataMutex.unlock();

        // Add the Command
        json["Cmd"] = "Data";               

        serialWriteJSON(json);
    } 

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
    } else if (strcmp(command, "Setttings") == 0) {        
        trkset.loadJSONSettings(json);
        serialWrite("HT: Saving Settings\r\n");           

    // Save to Flash
    } else if (strcmp(command, "Flash") == 0) {
        serialWrite("HT: Saving Settings\r\n");           
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
        uiResponsive = Kernel::Clock::now() + std::chrono::milliseconds(UIRESPONSIVE_TIME);
    
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
    }
}