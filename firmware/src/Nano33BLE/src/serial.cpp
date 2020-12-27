#include <Arduino.h>
#include <mbed.h>
#include "serial.h"

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