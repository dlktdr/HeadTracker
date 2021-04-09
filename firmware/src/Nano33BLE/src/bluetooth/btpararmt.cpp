#include <Arduino.h>
#include <ArduinoJson.h>
#include "ArduinoBLE.h"
#include "serial.h"
#include "io.h"
#include "main.h"
#include "trackersettings.h"
#include "opentxbt.h"
#include "btpararmt.h"

Timer watchdog;

#define WATCHDOG_TIMEOUT 10000

uint16_t chanoverrides=0xFFFF;
bool bleconnected=false;

void fff6Written(BLEDevice central, BLECharacteristic characteristic);
void overrideWritten(BLEDevice central, BLECharacteristic characteristic);

// Should be safe, never should be more than one instance of this
BTParaRmt *BTParaInst = nullptr;

BTParaRmt::BTParaRmt() : BTFunction()
{
    BTParaInst = this;
    bleconnected = false;
    chanoverrides = 0xFFFF;
    // Reset all BT channels to center
    for(int i = 0; i <16; i++)
        chan_vals[i] = TrackerSettings::PPM_CENTER;

    serialWriteln("HT: Starting Remote Para Bluetooth");

    // Save Local Address
    strcpy(_address,BLE.address().c_str());

    watchdog.start();
}

BTParaRmt::~BTParaRmt()
{
    BTParaInst = nullptr;
    // Reset all BT channels to center
    for(int i = 0; i <16; i++)
        chan_vals[i] = TrackerSettings::PPM_CENTER;

    serialWriteln("HT: Stopping Remote Para Bluetooth");

    // Disconnect
    BLE.disconnect();}

// Non-overridden bluetooth channels return 0
uint16_t BTParaRmt::getChannel(int channel)
{
    if(channel >= 0 && channel < BT_CHANNELS && bleconnected) {
        if((1 << channel) & chanoverrides) {
            return chan_vals[channel];
        }
    }

    return 0;
}

void BTParaRmt::execute()
{

    if(!bleconnected) {
        for(int i=0;i < BT_CHANNELS; i++)
            chan_vals[i] = TrackerSettings::PPM_CENTER;

    // If connected, set all non-overrides to center
    } else {
        for(int i=0;i < BT_CHANNELS; i++) {
            if(!(chanoverrides & (1<<i))) {
                chan_vals[i] = TrackerSettings::PPM_CENTER;
            }
        }
    }

     // Start Scan for PARA Slaves
    if(!BLE.connected() && !scanning) {
        serialWriteln("BLE: Starting Scan");
        BLE.scan();
        scanning = true;
        bleconnected = false;
    }

    // If scanning see if there is a BLE device available
    if(!BLE.connected() && scanning) {
        bool fault = false;
        peripheral = BLE.available();
        if(peripheral) {
#ifdef DEBUG
            if (peripheral.address() == "") {
                serialWrite("BLE:  <no advertised address> ");
            } else {
                serialWrite("BLE: Advertised Device Address: ");
                serialWrite(peripheral.address());
            }
            if (peripheral.localName() == "") {
                serialWrite(" <no advertised local name> ");
            } else {
                serialWrite(" Local Name: ");
                serialWrite(peripheral.localName());
            }

            if (peripheral.advertisedServiceUuid() == "") {
                serialWrite(" <no advertised service UUID> ");
            } else {
                serialWrite(" Advertised Service UUID ");
                serialWrite(peripheral.advertisedServiceUuid());
            }
            serialWriteln("");
#endif

            if(peripheral.localName() == "Hello" &&
               peripheral.advertisedServiceUuid() == "fff0") {

                serialWriteln("BLE: Found a PARA device");
#ifdef DEBUG
                serialWriteln("BLE: Stopping scan");
#endif
                BLE.stopScan();
                scanning = false;
                serialWriteln("BLE: Connecting...");
                if(peripheral.connect()) {
                    serialWriteln("BLE: Connected");
                    ThisThread::sleep_for(std::chrono::milliseconds(150));
#ifdef DEBUG
                    serialWriteln("BLE: Discovering Attributes");
#endif
                    if(peripheral.discoverAttributes()) {
                        serialWriteln("BLE: Discovered Attributes");
                        fff6 = peripheral.service("fff0").characteristic("fff6");
                        if(fff6) {
#ifdef DEBUG
                            serialWriteln("BLE: Attaching Event Handler");
#endif
                            fff6.setEventHandler(BLEWritten, fff6Written);  // Call this function on data received
                            serialWriteln("BLE: Found channel data characteristic");
#ifdef DEBUG
                            serialWriteln("BLE: Subscribing...");
#endif
                            ThisThread::sleep_for(std::chrono::milliseconds(150));
                            if(fff6.subscribe()) {
#ifdef DEBUG
                                serialWriteln("BLE: Subscribed to data!");
#endif
                            } else {
                                serialWriteln("BLE: Subscribe to data failed");
                                fault = true;
                            }
                        } else  {
                            serialWriteln("BLE: Couldn't find characteristic");
                            fault = true;
                        }
                        // If this is a Headtracker it may have a reset center option
                        butpress = peripheral.service("fff1").characteristic("fff2");
                        if(butpress) {
                            serialWriteln("BLE: Tracker has ability to remote reset center");
                        }
                        overridech = peripheral.service("fff1").characteristic("fff1");
                        if(overridech) {
                            overridech.setEventHandler(BLEWritten, overrideWritten);  // Call this function on data received
                            // Initial read of overridden channels
                            overridech.readValue(chanoverrides);
                            serialWriteln("BLE: Tracker has sent the channels it wants overriden");
                            ThisThread::sleep_for(std::chrono::milliseconds(150));
                            if(overridech.subscribe()) {
                                serialWriteln("BLE: Subscribed to channel overrides!");
                            } else {
                                serialWriteln("BLE: Subscribe to override Failed");
                                fault = true;
                            }
                        } else
                            chanoverrides = 0xFFFF;
                    } else {
                        serialWriteln("BLE: Attribute Discovery Failed");
                        fault = true;
                        }
                } else {
                    serialWriteln("BLE: Couldn't connect to Para Slave, Rescanning");
                    fault = true;
                }
            }
        }
        // On any faults, disconnect and start scanning again
        if(fault) {
            peripheral.disconnect();
            BLE.scan();
            scanning = true;
        }
    }

    // Connected
    if(BLE.connected()) {
        // Check how long we have been connected but haven't received any data
        // if longer than timeout, disconnect.
        uint32_t wdtime = std::chrono::duration_cast<std::chrono::milliseconds>(watchdog.elapsed_time()).count();
        if(wdtime > WATCHDOG_TIMEOUT) {
            serialWriteln("***WATCHDOG.. Forcing disconnect. No data received");
            BLE.disconnect();
            bleconnected = false;
        }
    // Not Connected
    } else {
        bleconnected = false;
        watchdog.reset();
    }

    BLE.poll();
}

void overrideWritten(BLEDevice central, BLECharacteristic characteristic)
{
    characteristic.readValue(chanoverrides);
}

// Called when Radio Outputs new Data
void fff6Written(BLEDevice central, BLECharacteristic characteristic) {
    // Got Data Must Be Connected
    bleconnected = true;

    // Got some data clear the watchdog timer
    watchdog.reset();

    uint8_t buffer1[BLUETOOTH_LINE_LENGTH+1];
    int len = characteristic.readValue(buffer1,32);

    // Simulate sending byte by byte like opentx uses, stores in global
    for(int i=0;i<len;i++) {
        processTrainerByte(buffer1[i]);
    }

    //
    for(int i=0 ; i < BT_CHANNELS; i++) {
        // Only set the data on channels that are allowed to be overriden
        if(chanoverrides & (1<<i)) {
            if(BTParaInst) {
                BTParaInst->chan_vals[i]  = BtChannelsIn[i];
            }
        }
    }

#ifdef DEBUG
    Serial.print("OR: ");
    printHex(chanoverrides);
    Serial.print("|");
    for(int i=0;i<CHANNEL_COUNT;i++) {
        Serial.print("Ch");Serial.print(i+1);Serial.print(":");Serial.print(ppmChannels[i]);Serial.print(" ");
    }
    Serial.println("");
#endif
}

void BTParaRmt::sendButtonData(char bd)
{
    if(butpress)
        butpress.writeValue((uint8_t)bd);
}