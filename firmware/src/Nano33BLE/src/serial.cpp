
#include "dataparser.h"

#include "serial.h"

// Really don't like this but for the life of me I couldn't get
// any ready to go buffered serial methods to work.
// Could have probably make a #define that overrides Serial.print

// Buffer
char jsondatabuf[RT_BUF_SIZE];

// Serial input / output Buffers
CircularBuffer<char, RT_BUF_SIZE> serin;
CircularBuffer<char, TX_BUF_SIZE> serout;

volatile bool JSONready;
volatile bool JSONfault;
volatile bool SerBufOverflow;
volatile bool BuffToSmall;

// Buffered write to serial
Mutex serWriteMutex;

void serial_Thread()
{ 
  
  while(1) {
    // Don't like this but can't get bufferedserial or available bytes to write
    // functions to work, at least shouldn't block this way.
    digitalWrite(LEDG,LOW); // Serial RX Green, ON
    int bytx = serout.size();
    char txa[60];
    bytx = MIN(bytx,60);
    for(int i =0;i<bytx;i++) {
      serout.pop(txa[i]);      
    }
    Serial.write(txa,bytx); // !!!! ONLY PLACE SERIAL.WRITE SHOULD BE USED !!!
    digitalWrite(LEDG,HIGH); // Serial RX Green, ON
    ThisThread::sleep_for(std::chrono::milliseconds(SERIAL_THREAD_PERIOD));
};}

// Serial RX Interrupt, Stay Fast as possible Here
void serialrx_Int()
{  
   digitalWrite(LEDG,LOW); // Serial RX Green, ON
  
  // Read all available data from Serial  
  while(Serial.available()) {
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
        char *dataptr = jsondatabuf;
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