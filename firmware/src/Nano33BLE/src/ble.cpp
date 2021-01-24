/* Much of this bluetooth PARA system interface was discovered by Yuri Soldak.
 *     https://github.com/ysoldak/HeadTracker
 */

#include <ArduinoBLE.h>
#include <ArduinoJson.h>
#include "dataparser.h"
#include "ble.h"
#include "PPM/PPMOut.h"
#include "serial.h"
#include "main.h"

using namespace events;

uint8_t sysid_data[8] = { 0xF1, 0x63, 0x1B, 0xB0, 0x6F, 0x80, 0x28, 0xFE };
uint8_t m_data[3] = { 0x41, 0x70, 0x70 };
uint8_t ieee_data[14] = { 0xFE, 0x00, 0x65, 0x78, 0x70, 0x65, 0x72, 0x69, 0x6D, 0x65, 0x6E, 0x74, 0x61, 0x6C };
uint8_t pnpid_data[7] = { 0x01, 0x0D, 0x00, 0x00, 0x00, 0x10, 0x01 };

BLEService info("180A");
BLECharacteristic sysid("2A23", BLERead, 8);
BLECharacteristic manufacturer("2A29", BLERead, 3);
BLECharacteristic ieee("2A2A", BLERead, 14);
BLECharacteristic pnpid("2A50", BLERead, 7);

BLEService para("FFF0");
BLEByteCharacteristic fff1("FFF1", BLERead | BLEWrite);
BLEByteCharacteristic fff2("FFF2", BLERead);
/*BLECharacteristic fff3("FFF3", BLEWriteWithoutResponse, 32);
BLECharacteristic fff5("FFF5", BLERead, 32);*/
BLECharacteristic fff6("FFF6", BLEWriteWithoutResponse | BLENotify, 32);

bool bleconnected = false;
bool oktowrite = true;

//-----------------------------------------------------
// FROM OPENTX 2.3 - bluetooth.cpp / .h


#define LEN_BLUETOOTH_ADDR              16
#define MAX_BLUETOOTH_DISTANT_ADDR      6
#define BLUETOOTH_LINE_LENGTH           32

constexpr uint8_t START_STOP = 0x7E;
constexpr uint8_t BYTE_STUFF = 0x7D;
constexpr uint8_t STUFF_MASK = 0x20;

uint8_t buffer[BLUETOOTH_LINE_LENGTH+1];
uint8_t bufferIndex = 0;
uint8_t crc=0;



int16_t limit(int16_t l, int16_t v, int16_t h) {return MAX(MIN((v),(h)),(l));}

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
    // Allocate Channel Mappings, Set Default to all Center
    uint16_t channelOutputs[12];
    for(int i=0;i < 12; i++) {
        channelOutputs[i] = 1500;
    }

    // Read Shared Data, skip if locked don't wait
    uint16_t to,ro,po;
    trkset.getPPMValues(to,ro,po);

    // Set Channel Values
    channelOutputs[trkset.panCh()-1] = po;
    channelOutputs[trkset.tiltCh()-1] = to;
    channelOutputs[trkset.rollCh()-1] = ro;

    uint8_t * cur = buffer; 
    bufferIndex = 0;
    crc = 0x00;

    buffer[bufferIndex++] = START_STOP; // start byte
    pushByte(0x80); // trainer frame type?
    for (int channel=0; channel<8; channel+=2, cur+=3) {
        
        uint16_t channelValue1 = channelOutputs[channel];
        uint16_t channelValue2 = channelOutputs[channel+1];
        
        pushByte(channelValue1 & 0x00ff);
        pushByte(((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4));
        pushByte(((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8));
    }
    
    buffer[bufferIndex++] = crc;
    buffer[bufferIndex++] = START_STOP; // end byte
        
    fff6.writeValue(buffer,bufferIndex);
    
    bufferIndex = 0;

}

//-----------------------------------------------------


void blePeripheralConnectHandler(BLEDevice central) 
{
    //pauseThreads = true;
    serialWrite("HT: Connected event, Name: ");
    serialWrite(central.localName().c_str());
    serialWrite(" Address: ");
    serialWriteln(central.address().c_str());   
    
    if (fff6.subscribe()) {
        serialWriteln("HT: Subscribed");
    }

    BLE.stopAdvertise();
    //pauseThreads = false;
    bleconnected = true;
}

void blePeripheralDisconnectHandler(BLEDevice central) 
{
   // pauseThreads = true;

    // Central Disconnected event handler
    serialWrite("HT: Disconnected event, central: ");
    serialWriteln(central.address().c_str());
    
    BLE.advertise();
    bleconnected = false;
    //pauseThreads = false;
}

void bt_Init() 
{
    if(!BLE.begin()) {
        serialWriteln("HT: Unable to start Bluetooth");    
    } else {
        serialWriteln("HT: Started BT");
    
    BLE.setConnectable(true);
    BLE.setLocalName("Hello");

    info.addCharacteristic(sysid);
    info.addCharacteristic(manufacturer);
    info.addCharacteristic(ieee);
    info.addCharacteristic(pnpid);

    BLE.addService(info);
    sysid.writeValue(sysid_data, 8);
    manufacturer.writeValue(m_data, 3);
    ieee.writeValue(ieee_data, 14);
    pnpid.writeValue(pnpid_data, 7);

    BLE.addService(para);
    BLE.setAdvertisedService(para);
    para.addCharacteristic(fff1);
    para.addCharacteristic(fff2);
    /*para.addCharacteristic(fff3);
    para.addCharacteristic(fff5);*/
    para.addCharacteristic(fff6);

   
    fff1.writeValue(0x01);
    fff2.writeValue(0x02);

    BLE.advertise();

    trkset.setBLEAddress(BLE.address().c_str());

    BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
  }
}

void bt_Thread() {
    if(pauseThreads) {
        queue.call_in(std::chrono::milliseconds(100),bt_Thread);
        return;
    }

    digitalWrite(LEDB,LOW);

    // Connected?
    if(BLE.central().connected()) 
        sendTrainer();

    digitalWrite(LEDB,HIGH);  
    
    queue.call_in(std::chrono::milliseconds(BT_PERIOD),bt_Thread);
}