#include "boardjson.h"
#include "ucrc16lib.h"

#ifndef WINDOWS
#define Sleep(x) sleep(x)
#endif

BoardJson::BoardJson(TrackerSettings *ts)
{
    trkset = ts;
    bleCalibratorDialog = new CalibrateBLE(trkset);

    connect(&imheretimout,SIGNAL(timeout()),this,SLOT(ihTimeout()));
    connect(&rxParamsTimer,SIGNAL(timeout()),this,SLOT(rxParamsTimeout()));
    rxParamsTimer.setSingleShot(true);
    connect(bleCalibratorDialog,&CalibrateBLE::calibrationSave,this,&BoardJson::calibrationComplete);
    connect(bleCalibratorDialog,&CalibrateBLE::calibrationCancel,this, &BoardJson::calibrationCancel);

    reqDataItemsChanged.setSingleShot(true);
    reqDataItemsChanged.setInterval(200);
    connect(&reqDataItemsChanged,SIGNAL(timeout()),this,SLOT(changeDataItems()));

    // BLE calibrator needs to be able to save the magnetometer selection
    connect(bleCalibratorDialog, &CalibrateBLE::saveToRam, this, &BoardJson::saveToRAM);
}

BoardJson::~BoardJson()
{
    delete bleCalibratorDialog;
}

void BoardJson::dataIn(QByteArray &data)
{
    // Found a SOT & EOT Character. JSON Data was sent
    if(data.left(1)[0] == (char)0x02 && data.right(1)[0] == (char)0x03) { // JSON Data
        QByteArray crcs = data.mid(data.length()-3,2);
        //uint16_t crc = crcs[0] << 8 | crcs[1];

        QByteArray stripped = data.mid(1,data.length()-4);
       // qDebug() << "JSONIn" << stripped;
        //uint crc2 = escapeCRC(uCRC16Lib::calculate(stripped.data(), stripped.length()));
        /*if(crc != crc2) {
            qDebug() << "CRC Fault" << crc << crc2;
        } else {*/
            parseIncomingJSON(QJsonDocument::fromJson(stripped).object().toVariantMap());
        //}

        //  Found the acknowldege Character, data was received without error
    } else if(data.left(1)[0] == (char)0x06) {
        // Clear the fault counter
        jsonwaitingack = 0;

        // If more data in queue, send it and wait for another ack char.
        if(!jsonqueue.isEmpty()) {
            serialDataOut += jsonqueue.dequeue();
            jsonwaitingack = 1;
            emit serialTxReady();            
        }

        // Found a not-acknowldege character, resend data
    } else if(data.left(1)[0] == (char)0x15) {
        nakError();

        // Other data sent, show the user
    } else {
        emit addToLog(data + "\n");
    }

}

QByteArray BoardJson::dataout()
{
    // Don't allow serial access if not allowed
    // prevents two boards talking at the same time

    // Return the serial data buffer
    QByteArray sdo = serialDataOut;
    serialDataOut.clear();
    return sdo;
}

void BoardJson::requestHardware()
{
    sendSerialJSON("FW"); // Get the firmware
}

void BoardJson::saveToRAM()
{
    QVariantMap d2s = trkset->changedData();
    // Remove useless items
    d2s.remove("axisremap");
    d2s.remove("axissign");
    d2s.remove("Hard");
    d2s.remove("Vers");
    // If no changes, return
    if(d2s.count() == 0)
        return;

    // Send Changed Data
    emit paramSendStart();
    sendSerialJSON("Set", d2s);

    // Set the data is now matched on the device
    trkset->setDataMatched();

    // Flag for exit, has data been sent
    savedToRAM = true;
    savedToNVM = false;
    paramTXErrorSent = false;
    paramRXErrorSent = false;
}

void BoardJson::saveToNVM()
{
    sendSerialJSON("Flash");
    savedToNVM = true;
}

void BoardJson::reboot()
{
  sendSerialJSON("Reboot");
}

void BoardJson::erase()
{
  sendSerialJSON("Erase");
}


// Parameters requested from the board

void BoardJson::requestParameters()
{
//    qDebug() << "JSON Data" << jsonqueue.length();
    if(rxparamfaults == 0) {
        emit paramReceiveStart();
    } else if (rxparamfaults > 3) {
        if(!paramRXErrorSent) {
            emit paramReceiveFailure(1);
            paramRXErrorSent = true;
        }
        return;
    }

    sendSerialJSON("Get"); // Get the Settings

    rxParamsTimer.stop();
    rxParamsTimer.start(800); // Start a timer, if we haven't got them, try again
}

// Timer if parameters are not received in time
void BoardJson::rxParamsTimeout()
{
    // Don't increment counter if data hasn't been sent yet
    if(serialDataOut.size() != 0) {
        rxParamsTimer.stop();
        rxParamsTimer.start(500); // Start a timer, if we haven't got them, try again
        return;
    } else {
        rxparamfaults++;
        requestParameters();
    }
}

// Function calls a timer, so if multiple emits in short order it only
// causes a single write.
void BoardJson::reqDataItemChanged()
{
    reqDataItemsChanged.stop();
    reqDataItemsChanged.start();
}

// Calibration Wizard not completed, remove calibration items

void BoardJson::calibrationCancel()
{
    // Remove all items
    trkset->clearDataItems();
    stopData();

    // Only request the ones that were active on calibration start
    trkset->setDataItemSend(cursendingdataitems);

    // Emit Calibration Fail
    emit calibrationFailure();
}

void BoardJson::calibrationComplete()
{
    // Remove all items
    trkset->clearDataItems();
    stopData();

    // Only request the ones that were active on calibration start
    trkset->setDataItemSend(cursendingdataitems);
    saveToRAM();
}

// Add/Remove data items to be received from the board
void BoardJson::changeDataItems()
{
    QMap<QString,bool> toChange = trkset->getDataItemsDiff();

    QVariantMap di;
    QMapIterator<QString, bool> i(toChange);
    while (i.hasNext()) {
        i.next();
        di[i.key()] = i.value();
    }
    sendSerialJSON("RD",di);

    // Notify the tracker settings that we have updated the board
    trkset->setDataItemsMatched();
}

void BoardJson::startCalibration()
{
    // Save a list if the currently sending data items
    //  to be restored on calibration completed/canceled
    cursendingdataitems = trkset->getDataItems();

    // Stop sending all data items
    trkset->clearDataItems();
    stopData();

    // Request just calibration items
    QMap<QString, bool> dat;
    dat["magx"] = true;
    dat["magy"] = true;
    dat["magz"] = true;
    dat["accx"] = true;
    dat["accy"] = true;
    dat["accz"] = true;
    trkset->setDataItemSend(dat);
    bleCalibratorDialog->show();
}

void BoardJson::startData()
{
    jsonqueue.clear();
    jsonwaitingack = 0;
}

void BoardJson::stopData()
{
    // Used on boot to reset data
    sendSerialJSON("D--"); // Stop all Data
}

void BoardJson::allowAccessChanged(bool acc)
{
    // Access was just allowed/disallowed to this class
    // reset everything
    calmsgshowed = false;
    savedToNVM=true;
    savedToRAM=true;
    paramTXErrorSent=false;
    paramRXErrorSent=false;
    serialDataOut.clear();
    jsonwaitingack =0;
    rxparamfaults=0;
    jsonqueue.clear();
    lastjson.clear();
    imheretimout.stop();
    updatesettingstmr.stop();
    rxParamsTimer.stop();    
    if(acc)
        connect(trkset,SIGNAL(requestedDataItemChanged()),this,SLOT(reqDataItemChanged()));
    else
        disconnect(trkset, SIGNAL(requestedDataItemChanged()), 0, 0);
}

void BoardJson::disconnected()
{
    bleCalibratorDialog->hide();
}

void BoardJson::resetCenter()
{
    sendSerialJSON("RstCnt");
}

void BoardJson::sendSerialJSON(QString command, QVariantMap map)
{
    // Don't send any new data until last has been received successfully
    map.remove("Hard");
    map.remove("Vers");

    QJsonObject jobj = QJsonObject::fromVariantMap(map);
    jobj["Cmd"] = command;
    QJsonDocument jdoc(jobj);
    QString json = QJsonDocument(jdoc).toJson(QJsonDocument::Compact);

    // Calculate the CRC Checksum
    uint16_t CRC = escapeCRC(uCRC16Lib::calculate(json.toUtf8().data(),json.length()));

    lastjson = (char)0x02 + json.toLatin1() + QByteArray::fromRawData((char*)&CRC,2) + (char)0x03 + "\r\n";
    //qDebug() << "JSONout" << lastjson;

    // If still waiting for an ack, add to the queue
    if(jsonwaitingack != 0) {
        jsonqueue.enqueue(lastjson);
        return;
    }

    // Add serial data to the TX buffer, emit a signal it's ready
    serialDataOut += lastjson;    
    jsonwaitingack = 1; // Set as faulted until ACK returned

    emit serialTxReady();

    // Reset Ack Timer
    imheretimout.stop();
    imheretimout.start(IMHERETIME);
}

void BoardJson::parseIncomingJSON(const QVariantMap &map)
{
    // Settings from the Tracker Sent, save them and update the UI
    if(map["Cmd"].toString() == "Set") {
        rxParamsTimer.stop(); // Stop error timer
        rxparamfaults = 0;
        trkset->setAllData(map);
        emit paramReceiveComplete();
        // Remind user to calibrate, if mag isn't disabled
        if(!map["dismag"].toBool()) {
            if(fabs(trkset->getMagXOff()) < 0.0001 &&
               fabs(trkset->getMagYOff()) < 0.0001 &&
               fabs(trkset->getMagZOff()) < 0.0001) {
                if(calmsgshowed == false) {
                    emit needsCalibration();
                    calmsgshowed = true;
                }
            }
        }

    // Data sent, Update the graph / servo sliders / calibration
    } else if (map["Cmd"].toString() == "Data") {
        QVariantMap cmap = map;


        // Decode Base64 encoded arrays
        int arrlength=0;
        for (QVariantMap::const_iterator it = cmap.cbegin(), end = cmap.cend(); it != end; ++it) {
            if(it.key().startsWith('6')) {
                QByteArray arr = QByteArray::fromBase64(it.value().toByteArray());                
                if(it.key().endsWith("u16")) {
                    const uint16_t *darray = ArrayType<uint16_t>::getData(arr,arrlength);
                    for(int i=0;i< arrlength;i++) {
                        trkset->setLiveData(it.key().mid(1,it.key().length()-4) + "[" +QString::number(i) + "]",darray[i]);
                    }
                } else if(it.key().endsWith("chr")) {
                    const char *darray = ArrayType<char>::getData(arr,arrlength);
                    trkset->setLiveData(it.key().mid(1,it.key().length()-4),darray);
                } else if(it.key().endsWith("u8")) {
                    const uint8_t *darray = ArrayType<uint8_t>::getData(arr,arrlength);
                    for(int i=0;i< arrlength;i++) {
                        trkset->setLiveData(it.key().mid(1,it.key().length()-3) + QString("[%1]").arg(i),darray[i]);
                    }
                } else if(it.key().endsWith("s16")) {
                    const int16_t *darray = ArrayType<int16_t>::getData(arr,arrlength);
                    for(int i=0;i< arrlength;i++) {
                        qDebug() << darray[i];
                    }
                } else if(it.key().endsWith("u32")) {
                    const uint32_t *darray = ArrayType<uint32_t>::getData(arr,arrlength);
                    for(int i=0;i< arrlength;i++) {
                        qDebug() << darray[i];
                    }
                } else if(it.key().endsWith("s32")) {
                    const int32_t *darray = ArrayType<int32_t>::getData(arr,arrlength);
                    for(int i=0;i< arrlength;i++) {
                        qDebug() << darray[i];
                    }
                } else if(it.key().endsWith("flt")) {
                    const float *darray = ArrayType<float>::getData(arr,arrlength);
                    for(int i=0;i< arrlength;i++) {
                        trkset->setLiveData(it.key().mid(1,it.key().length()-4) + QString("[%1]").arg(i),darray[i]);
                    }
                }                

                // Don't add it as encoded
                cmap.remove(it.key());
            }
        }

        // Add all the non array live data
        trkset->setLiveDataMap(cmap);

    // Firmware Hardware and Version
    } else if (map["Cmd"].toString() == "FW") {
        _boardName = map["Hard"].toString();
        trkset->setHardware(map["Vers"].toString(),
                            map["Hard"].toString(),
                            map["Git"].toString());
        emit boardDiscovered(this);
    }
}

uint16_t BoardJson::escapeCRC(uint16_t crc)
{
    // Characters to escape out
    uint8_t crclow = crc & 0xFF;
    uint8_t crchigh = (crc >> 8) & 0xFF;
    if(crclow == 0x00 ||
       crclow == 0x02 ||
       crclow == 0x03 ||
       crclow == 0x06 ||
       crclow == 0x15)
        crclow ^= 0xFF; //?? why not..
    if(crchigh == 0x00 ||
       crchigh == 0x02 ||
       crchigh == 0x03 ||
       crchigh == 0x06 ||
       crchigh == 0x15)
        crchigh ^= 0xFF; //?? why not..
    return (uint16_t)crclow | ((uint16_t)crchigh << 8);
}

void BoardJson::nakError()
{
    // If too many faults, disconnect.
    if(jsonwaitingack > MAX_TX_FAULTS) {
        if(!paramTXErrorSent) {
            emit addToLog("\r\nERROR: Critical - " + QString::number(MAX_TX_FAULTS)+ " transmission faults, disconnecting\r\n");
            emit paramSendFailure(1);
            paramTXErrorSent = true;
        }
    } else {
        // Pause a bit, give time for device to catch up
        Sleep(TX_FAULT_PAUSE);

        // Resend last JSON
        serialDataOut += lastjson;
        emit serialTxReady();
        emit addToLog("ERROR: CRC Fault - Re-sending data\r\nGUI: " +  lastjson + "\r\n");

        // Increment fault counter
        jsonwaitingack++;
    }
}

void BoardJson::ihTimeout()
{
    sendSerialJSON("IH");
}

