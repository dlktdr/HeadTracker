#include <ArduinoBLE.h>
#include "dataparser.h"
#include "ble.h"

void bt_Init() 
{
  if(!BLE.begin()) {
    serialWriteln("Unable to start Bluetooth");    
  } else {
    delay(1000);
    serialWriteln("started BT");
  }
  BLE.scan();
}

void bt_Thread() {
  BLEDevice peripheral;
  
  while(1) {
    digitalWrite(LEDR,LOW); // Green LED
    /* Add Bluetooth functionality Here */
     peripheral = BLE.available();

    if(peripheral) {
      serialWrite("Localname ");
      serialWrite(peripheral.localName()); 
      serialWrite(" address ");
      serialWrite(peripheral.address());
      serialWrite("\r\n");     
    } else {
      //serialWriteln("No Devices Found");
    }
    
    digitalWrite(LEDR,HIGH); // Serial RX Blue, Off
    ThisThread::sleep_for(std::chrono::milliseconds(200));
  }
}