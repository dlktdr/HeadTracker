#include "boardnano33ble.h"

BoardNano33BLE::BoardNano33BLE(TrackerSettings *ts)
{
    trkset = ts;
    bleCalibratorDialog = new CalibrateBLE(trkset);

    connect(&imheretimout,SIGNAL(timeout()),this,SLOT(ihTimeout()));
    connect(&rxParamsTimer,SIGNAL(timeout()),this,SLOT(rxParamsTimeout()));
    rxParamsTimer.setSingleShot(true);
    connect(bleCalibratorDialog,&CalibrateBLE::calibrationSave,this,&BoardNano33BLE::saveToRAM);
}

BoardNano33BLE::~BoardNano33BLE()
{
    delete bleCalibratorDialog;
}

void BoardNano33BLE::dataIn(QByteArray &data)
{
    // Found a SOT & EOT Character. JSON Data was sent
    if(data.left(1)[0] == (char)0x02 && data.right(1)[0] == (char)0x03) { // JSON Data
        QByteArray crcs = data.mid(data.length()-3,2);
        //uint16_t crc = crcs[0] << 8 | crcs[1];

        QByteArray stripped = data.mid(1,data.length()-4);
        //uint crc2 = escapeCRC(uCRC16Lib::calculate(stripped.data(), stripped.length()));
        /*if(crc != crc2) {
            qDebug() << "CRC Fault" << crc << crc2;
        } else {*/
            parseIncomingJSON(QJsonDocument::fromJson(stripped).object().toVariantMap());
        //}

        //  Found the acknowldege Character, data was received without error
    } else if(data.left(1)[0] == (char)0x06) {
        // Clear the fault counter
        jsonfaults = 0;

        // If more data in queue, send it and wait for another ack char.
        if(!jsonqueue.isEmpty()) {
            serialDataOut += jsonqueue.dequeue();
            emit serialTxReady();
            jsonfaults = 1;
        }

        // Found a not-acknowldege character, resend data
    } else if(data.left(1)[0] == (char)0x15) {
        comTimeout();

        // Other data sent, show the user
    } else {
        emit addToLog(data + "\n");
    }

}

QByteArray BoardNano33BLE::dataout()
{
    // Don't allow serial access if not allowed
    // prevents two boards talking at the same time

    // Return the serial data buffer
    QByteArray sdo = serialDataOut;
    serialDataOut.clear();
    return sdo;
}

void BoardNano33BLE::requestHardware()
{
    sendSerialJSON("FW"); // Get the firmware    
    sendSerialJSON("IH"); // Start Data Transfer right away (Im here)
}

void BoardNano33BLE::saveToRAM()
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
}

void BoardNano33BLE::saveToNVM()
{
    sendSerialJSON("Flash");
    savedToNVM = true;
}


// Parameters requested from the board

void BoardNano33BLE::requestParameters()
{
    qDebug() << "JSON Data" << jsonqueue.length();
    if(rxparamfaults == 0) {
        emit paramReceiveStart();
    } else if (rxparamfaults > 3) {
        emit paramReceiveFailure(1);
        return;
    }

    sendSerialJSON("Get"); // Get the Settings

    rxParamsTimer.stop();
    rxParamsTimer.start(800); // Start a timer, if we haven't got them, try again
}

// Timer if parameters are not received in time
void BoardNano33BLE::rxParamsTimeout()
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


void BoardNano33BLE::startCalibration()
{
    bleCalibratorDialog->show();
}

void BoardNano33BLE::allowAccessChanged(bool acc)
{
    Q_UNUSED(acc);

    // Access was just allowed/disallowed to this class
    // reset everything
    calmsgshowed = false;
    savedToNVM=true;
    savedToRAM=true;
    serialDataOut.clear();
    jsonfaults =0;
    rxparamfaults=0;
    jsonqueue.clear();
    lastjson.clear();
    imheretimout.stop();
    updatesettingstmr.stop();
    rxParamsTimer.stop();
}

void BoardNano33BLE::disconnected()
{
    bleCalibratorDialog->hide();
}

void BoardNano33BLE::resetCenter()
{
    sendSerialJSON("RstCnt");
}

void BoardNano33BLE::sendSerialJSON(QString command, QVariantMap map)
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

    lastjson = (char)0x02 + json.toLatin1() + QByteArray::fromRawData((char*)&CRC,2) + (char)0x03;

    // If there is data that didn't make it there yet push this data to the queue
    // to be sent later
    if(jsonfaults != 0) {
        jsonqueue.enqueue(lastjson);
        emit serialTxReady();
        return;
    }

    // Add serial data to the TX buffer, emit a signal it's ready
    serialDataOut += lastjson;
    emit serialTxReady();

    // Set as faulted until ACK returned
    jsonfaults = 1;

    // Reset Ack Timer
    imheretimout.stop();
    imheretimout.start(IMHERETIME);
}

void BoardNano33BLE::parseIncomingJSON(const QVariantMap &map)
{
    // Settings from the Tracker Sent, save them and update the UI
    if(map["Cmd"].toString() == "Set") {
        rxParamsTimer.stop(); // Stop error timer
        rxparamfaults = 0;
        trkset->setAllData(map);
        emit paramReceiveComplete();        

    // Data sent, Update the graph / servo sliders / calibration
    } else if (map["Cmd"].toString() == "Data") {
        // Add all the data to the settings
        trkset->setLiveDataMap(map);

        // Remind user to calibrate
        if(map["magcal"].toBool() == false && calmsgshowed == false) {
            emit needsCalibration();
            calmsgshowed = true;
        }

    // Firmware Hardware and Version
    } else if (map["Cmd"].toString() == "FW") {
        trkset->setHardware(map["Vers"].toString(),map["Hard"].toString());
        emit boardDiscovered(this);
    }
}

uint16_t BoardNano33BLE::escapeCRC(uint16_t crc)
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

void BoardNano33BLE::comTimeout()
{
    // If too many faults, disconnect.
    if(jsonfaults > MAX_TX_FAULTS) {
        emit addToLog("\r\nERROR: Critical - " + QString(MAX_TX_FAULTS)+ " transmission faults, disconnecting\r\n");
        emit paramSendFailure(1);

    } else {
        // Pause a bit, give time for device to catch up
        // **** Sleep(TX_FAULT_PAUSE);

        // Resend last JSON
        serialDataOut += lastjson;
        emit serialTxReady();
        emit addToLog("ERROR: CRC Fault - Re-sending data (" +  lastjson + ")\n");
    }

    // Increment fault counter
    jsonfaults++;
}

void BoardNano33BLE::ihTimeout()
{
    sendSerialJSON("IH");
}

