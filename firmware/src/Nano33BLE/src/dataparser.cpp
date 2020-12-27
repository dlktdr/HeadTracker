#include <Arduino.h>
#include <mbed.h>
#include <rtos.h>
#include <chrono >
#include <platform/CircularBuffer.h>
#include "HardwareSerial.h"
#include <ArduinoJson.h>
#include "dataparser.h"
#include "main.h"

using namespace rtos;
using namespace mbed;

// Buffers for Serial Sending, Receiving and JSON Data
static const int RT_BUF_SIZE=1000; // RX Buffer Size
static const int TX_BUF_SIZE=1000; // RX Buffer Size
static const int DATA_THREAD_PERIOD = 10; // 10ms Period, Data Thread, Apx.
static const int DATA_SEND_TIME=100; // Data Update Speed in ms
static const int UIRESPONSIVE_TIME=10000; // 10Seconds without an ack data will stop;
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
Kernel::Clock::time_point uiResponsive = Kernel::Clock::now() + std::chrono::milliseconds(UIRESPONSIVE_TIME);

bool sendData=false;

// Buffered write to serial
Mutex serWriteMutex;

void serialWriteJSON(DynamicJsonDocument &json)
{  
  serWriteMutex.lock(); 
  
  char data[500];
  int br = serializeJson(json, data, 500); // Serialize, with max 500 char buffer

  serout.push(0x02);
  // Append Output to the serial output buffer
  for(int i =0; i < br; i++) {
    serout.push(data[i]);
  }
  serout.push(0x03);
  serout.push('\r');
  serout.push('\n');
  
  serWriteMutex.unlock();  
}

void serialWrite(arduino::String str)
{
  serialWrite(str.c_str());
}

void serialWriteln(char const *data)
{
  serWriteMutex.lock();
  int br = strlen(data);
  // Append Output to the serial output buffer
  for(int i =0; i < br; i++) {
    serout.push(data[i]);
  }
  serout.push('\r');
  serout.push('\n');
  serWriteMutex.unlock();
}

void serialWrite(int val) {
  serWriteMutex.lock();
  char buf[50];
  itoa(val,buf,10);
  int len = strlen(buf);
  // Append Output to the serial output buffer
  for(int i =0; i < len; i++) {
    serout.push(buf[i]);
  }
  serWriteMutex.unlock();
}

void serialWrite(char *data,int len) {
  serWriteMutex.lock();
  // Append Output to the serial output buffer
  for(int i =0; i < len; i++) {
    serout.push(data[i]);
  }
  serWriteMutex.unlock();
}


void serialWrite(char const *data) {
  serWriteMutex.lock();
  int br = strlen(data);
  // Append Output to the serial output buffer
  for(int i =0; i < br; i++) {
    serout.push(data[i]);
  }
  serWriteMutex.unlock();
}

void serialWrite(char c) {
  serWriteMutex.lock();
  serout.push(c);
  serWriteMutex.unlock();
}

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
  DynamicJsonDocument json(RT_BUF_SIZE);

  while(1) {
    digitalWrite(LEDB,LOW); 
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

    if(dataSendTime < curtime) {
        //} &&    uiResponsive > curtime) {
      
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
#ifdef DEBUG_HT
      serialWrite("HT: Resetting Center\r\n");
#endif
      pressButton();

    // Store Settings
    } else if (strcmp(command, "SetValues") == 0) {
#ifdef DEBUG_HT      
        serialWrite("HT: Saving Settings\r\n");           
#endif
        trkset.loadJSONSettings(json);
    
    // Get settings
    } else if (strcmp(command, "GetSet") == 0) {  
        serialWrite("HT: Sending Settings\r\n");
        json.clear();
        trkset.setJSONSettings(json);
        json["Command"] = "Settings";
        serialWriteJSON(json);

    // ACK Received, Means the GUI is running, send it data
    } else if (strcmp(command, "ACK") == 0) {
#ifdef DEBUG_HT
        serialWrite("HT: Ack Received\r\n");
#endif
        uiResponsive = Kernel::Clock::now() + std::chrono::milliseconds(UIRESPONSIVE_TIME);
        dataSendTime = Kernel::Clock::now() + std::chrono::milliseconds(DATA_SEND_TIME);
    
    // Version Requested
    } else if (strcmp(command, "FW") == 0) {
#ifdef DEBUG_HT
        serialWrite("HT: Sending FW Version\r\n");
#endif
      json.clear();
      json["Command"] = "FW";
      json["Mag_Ver"] = FW_MAJ_VERSION;
      json["Min_Ver"] = FW_MIN_VERSION;
      json["Board"] = FW_BOARD;
      serialWriteJSON(json);

    // Unknown Command
    } else {
        serialWrite("HT: Unknown Command\r\n");
    
    }
}