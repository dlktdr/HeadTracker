
#include "dataparser.h"

#include "serial.h"
#include "main.h"

// Really don't like this but for the life of me I couldn't get
// any ready to go buffered serial methods to work.
// Could have probably make a #define that overrides Serial.print

// Buffer, Store up to RX_BUFFERS JSON Messages before loosing data.
char jsondatabuf[RX_BUFFERS][RX_BUF_SIZE];

// Serial input / output Buffers
CircularBuffer<char, RX_BUF_SIZE> serin;
CircularBuffer<char, TX_BUF_SIZE> serout;
Mutex writingBufffer;

volatile bool JSONready;
volatile bool JSONfault;
volatile bool SerBufOverflow;
volatile bool BuffToSmall;
volatile int bufIndex=0;
volatile int bufsUsed=0;

// Buffered write to serial
Mutex serWriteMutex;

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

    // Don't like this but can't get bufferedserial to work and the Arduino serial lib has blocking writes after buffer fill.
    // Also no notify if its full or not...
    digitalWrite(LEDG,LOW); // Serial RX Green, ON
    int bytx = serout.size();
    char txa[SERIAL_TX_MAX_PACKET];
    bytx = MIN(bytx,SERIAL_TX_MAX_PACKET); // Packet size for USB FullSpeed is 64bytes..
    // Now if only I could tell when the packet is gone...
    // probably have to look at the nrf datasheet.

    for(int i =0;i<bytx;i++) {
      serout.pop(txa[i]);
    }
    Serial.write(txa,bytx); // !!!! ONLY PLACE SERIAL.WRITE SHOULD BE USED !!!!
    digitalWrite(LEDG,HIGH); // Serial RX Green, ON
    queue.call_in(std::chrono::milliseconds(SERIAL_PERIOD),serial_Thread);
}

// Pop a JSON item off the buffer
char *getJSONBuffer()
{
    __disable_irq();
    int bi = bufIndex;
    int bu = bufsUsed;
    bufsUsed--;
    __enable_irq();

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

    // Read all available data from Serial
    for(int i=0; i < bytes; i++) {
        char sc = Serial.read();
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

  char data[TX_BUF_SIZE];
  int br = serializeJson(json, data, TX_BUF_SIZE); // Serialize, with max 500 char buffer

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