#include <Arduino.h>
#include <mbed.h>
#include "serial.h"
#include "dataparser.h"

// Really don't like this but for the life of me I couldn't get
// any ready to go buffered serial methods to work.
// Could have probably make a #define that overrides Serial.print


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
  serWriteMutex.lock();
  for(int i =0; i < br; i++) {
    serout.push(data[i]);
  }
  serout.push('\r');
  serout.push('\n');
  serWriteMutex.unlock();
}

void serialWrite(int val) 
{ 
  char buf[50];
  itoa(val,buf,10);
  int len = strlen(buf);
  // Append Output to the serial output buffer
  serWriteMutex.lock();
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
  
  int br = strlen(data);
  // Append Output to the serial output buffer
  serWriteMutex.lock();
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