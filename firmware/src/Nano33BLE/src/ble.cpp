#include <ArduinoBLE.h>
#include "dataparser.h"
#include "ble.h"

void bt_Init() 
{
  if(!BLE.begin()) {
    serialWriteln("Unable to start Bluetooth");    
  }

  BLE.scan();
}

void bt_Thread() {
  while(1) {
    /* Add Bluetooth functionality Here */
    BLEDevice peripheral = BLE.available();

    if(peripheral) {
      if(strcmp(peripheral.localName().c_str(),"Hello") == 0) {
        serialWrite("Found a OpenTX Remote at");
        serialWrite(peripheral.address());
        serialWrite("\r\n");
      }
    }

    ThisThread::sleep_for(std::chrono::milliseconds(500));
  }
};