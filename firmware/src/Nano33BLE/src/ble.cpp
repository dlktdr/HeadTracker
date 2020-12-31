#include <ArduinoBLE.h>
#include <ArduinoJson.h>
#include "dataparser.h"
#include "ble.h"
#include "PPMOut.h"
#include "main.h"

BLEDevice peripheral;
BLEService bleservice("FFF0");
BLEService bleservice2("180A");
BLECharacteristic s2c1("2A50", BLERead,10);

BLECharacteristic blecar1("FFF1", BLERead | BLEWrite, 60);
BLECharacteristic blecar2("FFF2", BLERead , 60);
BLECharacteristic blecar3("FFF3", BLEWriteWithoutResponse, 60);
BLECharacteristic blecar5("FFF5", BLERead , 60);
BLECharacteristic blecar6("FFF6", BLENotify | BLEWriteWithoutResponse | BLEWrite, 20);
bool bleconnected = false;
bool oktowrite = true;

//-----------------------------------------------------
// FROM OPENTX 2.3 - bluetooth.cpp / .h

#define limit(l,v,h) (MAX(MIN( v , h ), l ))
#define LEN_BLUETOOTH_ADDR              16
#define MAX_BLUETOOTH_DISTANT_ADDR      6
#define BLUETOOTH_LINE_LENGTH           32

constexpr uint8_t START_STOP = 0x7E;
constexpr uint8_t BYTE_STUFF = 0x7D;
constexpr uint8_t STUFF_MASK = 0x20;

uint8_t buffer[BLUETOOTH_LINE_LENGTH+1];
uint8_t bufferIndex = 0;
uint8_t crc=0;

void pushByte(uint8_t byte)
{
  crc ^= byte;
  if (byte == START_STOP || byte == BYTE_STUFF) {
    buffer[bufferIndex++] = BYTE_STUFF;
    byte ^= STUFF_MASK;
  }
  buffer[bufferIndex++] = byte;
}

void sendTrainer()
{
  int16_t PPM_range = 512*2;  
  
  int firstCh = 0;
  int lastCh = firstCh + 8;
  
  // Allocate Channel Mappings, Set Default to all Center
  uint16_t channelOutputs[24];
  for(int i=0;i < 24; i++) {
    channelOutputs[i] = TrackerSettings::DEF_CENTER;
  }
  
  // Read Shared Data
  dataMutex.lock();
  uint16_t to,ro,po;
  trkset.getPPMValues(to,ro,po);
    // Set Channel Values
  channelOutputs[trkset.panCh()+firstCh] = po;
  channelOutputs[trkset.tiltCh()+firstCh] = to;
  channelOutputs[trkset.rollCh()+firstCh] = ro;
  dataMutex.unlock();
  
  uint8_t * cur = buffer;
  bufferIndex = 0;
  crc = 0x00;

  buffer[bufferIndex++] = START_STOP; // start byte
  pushByte(0x80); // trainer frame type?
  for (int channel=0; channel<lastCh; channel+=2, cur+=3) {
    
    uint16_t channelValue1 = ((channelOutputs[channel] - TrackerSettings::DEF_CENTER) /2) + TrackerSettings::DEF_CENTER;
    uint16_t channelValue2 = ((channelOutputs[channel+1] - TrackerSettings::DEF_CENTER) /2) + TrackerSettings::DEF_CENTER;
    
    //uint16_t channelValue1 = PpmOut::PPM_CENTER + limit((int16_t)-PPM_range, (int16_t)ppmout->getChannel(channel) - PpmOut::PPM_CENTER, (int16_t)PPM_range) / 2;
    //uint16_t channelValue2 = PpmOut::PPM_CENTER + limit((int16_t)-PPM_range, (int16_t)ppmout->getChannel(channel+1) - PpmOut::PPM_CENTER, (int16_t)PPM_range) / 2;
    pushByte(channelValue1 & 0x00ff);
    pushByte(((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4));
    pushByte(((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8));
  }
  
  buffer[bufferIndex++] = crc;
  buffer[bufferIndex++] = START_STOP; // end byte
  
  
  // Write to characteristic
  blecar6.writeValue(buffer,bufferIndex);
  

  bufferIndex = 0;
}

//-----------------------------------------------------


void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  serialWrite("Connected event, central: ");
  serialWriteln(central.address().c_str());   
  bleconnected = true;
}

void blePeripheralDisconnectHandler(BLEDevice central) 
{
  // central disconnected event handler
  serialWrite("Disconnected event, central: ");
  serialWriteln(central.address().c_str());
  bleconnected = false;
}

void characteristicRead(BLEDevice central, BLECharacteristic characteristic) 
{
  serialWrite("CR "); serialWrite(characteristic.uuid()); serialWrite(" Read ");  
}

void characteristicWritten(BLEDevice central, BLECharacteristic characteristic) 
{
  serialWrite("Characteristic event, written: ");

  char ffer[200];
  int i= blecar1.valueLength();
  characteristic.readValue(ffer,i++);
  ffer[i] = '\0';
  serialWrite("CR "); serialWrite(characteristic.uuid()); serialWrite(" ");
  serialWrite(ffer);    serialWrite("\r\n");
}

void bt_Init() 
{
  if(!BLE.begin()) {
    serialWriteln("Unable to start Bluetooth");    
  } else {
    serialWriteln("Started BT");
    

    bleservice.addCharacteristic(blecar1);
    bleservice.addCharacteristic(blecar2);
    bleservice.addCharacteristic(blecar3);
    bleservice.addCharacteristic(blecar5);
    bleservice.addCharacteristic(blecar6);
    bleservice2.addCharacteristic(s2c1);

    BLE.setAdvertisedService(bleservice);
    BLE.addService(bleservice);
    BLE.addService(bleservice2);
    BLE.setLocalName("Head Tracker");
    

    // assign event handlers for connected, disconnected to peripheral
    BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
    blecar1.setEventHandler(BLEWritten, characteristicWritten);
    blecar1.setEventHandler(BLERead, characteristicRead);
    
    blecar2.setEventHandler(BLEWritten, characteristicWritten);
    blecar2.setEventHandler(BLERead, characteristicRead);
    
    blecar3.setEventHandler(BLEWritten, characteristicWritten);
    blecar3.setEventHandler(BLERead, characteristicRead);
    
    blecar5.setEventHandler(BLEWritten, characteristicWritten);
    blecar5.setEventHandler(BLERead, characteristicRead);
    
    blecar6.setEventHandler(BLEWritten, characteristicWritten);
    blecar6.setEventHandler(BLERead, characteristicRead);
    
    BLE.setConnectable(true);
    BLE.advertise();
  }
}

void bt_Thread() {
  int counter=0;
  while(1) {    

    if(pauseForEEPROM) {
      ThisThread::sleep_for(std::chrono::milliseconds(100)); 
      continue;
    }

    digitalWrite(LEDR,LOW);
    
    // Blue tooth connected send the trainer information
    if(BLE.connected() && counter > 2) { 
      sendTrainer();
      counter = 0;
    } else 
      counter++;

    // Poll BT and Handle    
    BLE.poll();   
        
    digitalWrite(LEDR,HIGH);  

    // Can't Seem to go faster than apx here without crashing the thread???
    // Caused by the write in sendTrainer to the characteristic.
    // I assume the last write hasn't been completed but not sure how to check.
    ThisThread::sleep_for(std::chrono::milliseconds(50)); 
  }
}