#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <platform/CircularBuffer.h>
#include <ArduinoJson.h>
#include "dataparser.h"

using namespace rtos;
using namespace mbed;

// Buffers for Serial Sending, Receiving and JSON Data
const int RT_BUF_SIZE=1000; // Will use 3x this many bytes
const int DATA_THREAD_PERIOD = 10; // 10ms Period on Data Thread
const int DATA_SEND_RATE=10; // x10 multipler on send of data to UI
const int DATA_SEND_TIMEOUT=200; // After xx cycles without PC connection stop TXing data
char tempdata[RT_BUF_SIZE];

CircularBuffer<char, RT_BUF_SIZE> serin;

// Booleans for if JSON has been received - ISR flags
volatile bool JSONready=false;
volatile bool JSONfault=false;
volatile bool SerBufOverflow=false;
volatile bool BuffToSmall=false;

void parseData(DynamicJsonDocument &json);

// Tracker wide settings.
TrackerSettings trkset;

// Serial RX Interrupt
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
      
    } else { // Add data to buffer
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
    int data_rate_cnt=0;
    DynamicJsonDocument json(RT_BUF_SIZE);

    while(1) {
        // Check if data is ready        
        if(JSONready) {
            DeserializationError de = deserializeJson(json, tempdata);      
            if(de) {
                    Serial.println("deserializeJson() failed: ");
                    //Serial.println(de.f_str());
            }

            // ISR Flags
            __disable_irq();
            JSONready = false;
            __enable_irq();

            parseData(json);            
        } 

        if(JSONfault) {
        Serial.println("JSON Data Lost");

        // ISR flags
        __disable_irq();
        JSONready = false;
        JSONfault = false;
        __enable_irq();
        }

        if(SerBufOverflow) {
        Serial.println("Serial Buffer Overflow!");

        // ISR Flags
        __disable_irq();
        SerBufOverflow = false;
        JSONready = false;
        JSONfault = false;
        __enable_irq();
        }

        // Automatically send the UI Updates 
        if(data_rate_cnt++ == DATA_SEND_RATE) {
            
            data_rate_cnt = 0;
        }

        ThisThread::sleep_for(std::chrono::milliseconds(50));      
    }
}

// New JSON data received from the PC
void parseData(DynamicJsonDocument &json)
{    
    const char *command = json["Command"];
    
    // Reset Center
    if(strcmp(command,"Reset_Center") == 0) {
        Serial.println("Resetting Center");
       
    // Store Settings
    } else if (strcmp(command, "SetValues") == 0) {
        Serial.println("Saving Settings");           
        trkset.setFromJSON(json);
    
    } else if (strcmp(command, "GetSet") == 0) {
        Serial.println("Sending Settings");
        json.clear();
        trkset.setToJSON(json);
        json["Command"] = "Settings";
        // SOT char
        Serial.print((char)0x02);
        serializeJson(json, Serial);
        // EOT char
        Serial.println((char)0x03);        
                
    // Start Calibration
    } else if (strcmp(command, "CalStart") == 0) {
        Serial.println("Starting Calibration");        
    
    // Unknown Command
    } else {
        Serial.println("Unknown Command");
    
    } 

}