#include "boardbno055.h"

BoardBNO055::BoardBNO055(TrackerSettings *ts)
{
    trkset = ts;
    savedToNVM=true;
    savedToRAM=true;

    bnoCalibratorDialog = new CalibrateBNO;
}

BoardBNO055::~BoardBNO055()
{
    delete bnoCalibratorDialog;
}

void BoardBNO055::dataIn(QByteArray &data)
{
    // Don't allow serial access if not allowed
    if(!isAccessAllowed()) return;

    if(data.left(1) == "$") {
        parseIncomingHT(data);
    } else {
        emit addToLog(data + "\n");
    }

}

QByteArray BoardBNO055::dataout()
{
    if(!isAccessAllowed()) {
        serialDataOut.clear();
        return QByteArray();
    }

    // Return the serial data buffer
    QByteArray sdo = serialDataOut;
    serialDataOut.clear();
    return sdo;
}

void BoardBNO055::disconnected()
{
    stopGraph();
    bnoCalibratorDialog->hide();
    serialDataOut.clear();
}

void BoardBNO055::resetCenter()
{
    serialDataOut += "$RST\r\n";
    emit serialTxReady();
}

void BoardBNO055::requestHardware()
{
    serialDataOut += "$HARD\r\n";
    serialDataOut += "$VERS\r\n";
    emit serialTxReady();
}

void BoardBNO055::saveToRAM()
{
    QStringList lst;
    lst.append(QString::number(trkset->gyroWeightTiltRoll()));
    lst.append(QString::number(trkset->gyroWeightPan()));
    lst.append(QString::number(trkset->getTlt_Gain()*10));
    lst.append(QString::number(trkset->getPan_Gain()*10));
    lst.append(QString::number(trkset->getRll_Gain()*10));
    lst.append(QString::number(trkset->getServoReverse()));
    lst.append(QString::number(trkset->getPan_Cnt()));
    lst.append(QString::number(trkset->getPan_Min()));
    lst.append(QString::number(trkset->getPan_Max()));
    lst.append(QString::number(trkset->getTlt_Cnt()));
    lst.append(QString::number(trkset->getTlt_Min()));
    lst.append(QString::number(trkset->getTlt_Max()));
    lst.append(QString::number(trkset->getRll_Cnt()));
    lst.append(QString::number(trkset->getRll_Min()));
    lst.append(QString::number(trkset->getRll_Max()));
    lst.append(QString::number(trkset->getPanCh()));
    lst.append(QString::number(trkset->getTltCh()));
    lst.append(QString::number(trkset->getRllCh()));
    lst.append(QString::number(trkset->axisRemap()));
    lst.append(QString::number(trkset->axisSign()));
    QString data = lst.join(',');

    // Calculate the CRC Checksum
    uint16_t CRC = escapeCRC(uCRC16Lib::calculate(data.toUtf8().data(),data.length()));

    // Append Data in a Byte Array
    QByteArray bd = "$" + QString(data).toLatin1() + QByteArray::fromRawData((char*)&CRC,2) + "HE";

    serialDataOut += bd + "\r\n";
    emit serialTxReady();
}

void BoardBNO055::saveToNVM()
{
    // Not used on BNO
}

void BoardBNO055::requestParameters()
{
    serialDataOut += "$GSET\r\n";
    emit serialTxReady();
    emit paramReceiveStart();
}

void BoardBNO055::startCalibration()
{
    bnoCalibratorDialog->show();
}

void BoardBNO055::startData()
{
    startGraph();
}

void BoardBNO055::allowAccessChanged(bool acc)
{
    Q_UNUSED(acc)
    savedToNVM=true;
    savedToRAM=true;
    vers = QString();
    hard = QString();
}

void BoardBNO055::parseIncomingHT(QString cmd)
{
    static int fails=0;

    // CRC ERROR
    if(cmd.left(7) == "$CRCERR") {
        addToLog("Headtracker CRC Error!\n");
        emit statusMessage("CRC Error : Error Setting Values, Retrying");
        if(fails++ == 3)
            emit paramSendFailure(1);
        else
            saveToRAM();
    }

    // CRC OK
    else if(cmd.left(6) == "$CRCOK") {
        emit statusMessage("Values Set On Headtracker",2000);
        emit paramSendComplete();
        fails = 0;
        savedToRAM=true;
    }

    // Calibration Saved
    else if(cmd.left(7) == "$CALSAV") {
        emit statusMessage("Calibration Saved", 2000);
    }

    // Graph Data
    else if(cmd.left(2) == "$G") {
        cmd = cmd.mid(2).simplified();
        QStringList rtd = cmd.split(',');
        if(rtd.length() == 10) {
            QVariantMap vm;
            if(rawmode) {
                vm["tilt"] = rtd.at(0);
                vm["roll"] = rtd.at(1);
                vm["pan"] = rtd.at(2);
            } else {
                vm["tiltoff"] = rtd.at(0);
                vm["rolloff"] = rtd.at(1);
                vm["panoff"] = rtd.at(2);
            }
            vm["panout"] = rtd.at(3);
            vm["tiltout"] = rtd.at(4);
            vm["rollout"] = rtd.at(5);
            vm["syscal"] = rtd.at(6);
            vm["gyrocal"] = rtd.at(7);
            vm["accelcal"] = rtd.at(8);
            vm["magcal"] = rtd.at(9);
            trkset->setLiveDataMap(vm);
            graphing = true;
            bnoCalibratorDialog->setCalibration(vm["syscal"].toInt(),
                                             vm["magcal"].toInt(),
                                             vm["gyrocal"].toInt(),
                                             vm["accelcal"].toInt());
        }
    }
    // Setting Data
    else if(cmd.left(5) == "$SET$") {
        QStringList setd = cmd.right(cmd.length()-5).split(',',Qt::KeepEmptyParts);
        if(setd.length() == 22) {
            trkset->setGyroWeightTiltRoll(setd.at(2).toFloat());
            trkset->setGyroWeightPan(setd.at(3).toFloat());
            trkset->setTlt_Gain(setd.at(4).toFloat() /10);
            trkset->setPan_Gain(setd.at(5).toFloat()/10);
            trkset->setRll_Gain(setd.at(6).toFloat()/10);
            trkset->setServoReverse(setd.at(7).toInt());
            trkset->setPan_Cnt(setd.at(8).toInt());
            trkset->setPan_Min(setd.at(9).toInt());
            trkset->setPan_Max(setd.at(10).toInt());
            trkset->setTlt_Cnt(setd.at(11).toInt());
            trkset->setTlt_Min(setd.at(12).toInt());
            trkset->setTlt_Max(setd.at(13).toInt());
            trkset->setRll_Cnt(setd.at(14).toInt());
            trkset->setRll_Min(setd.at(15).toInt());
            trkset->setRll_Max(setd.at(16).toInt());
            trkset->setPanCh(setd.at(17).toInt());
            trkset->setTltCh(setd.at(18).toInt());
            trkset->setRllCh(setd.at(19).toInt());
            trkset->setAxisRemap(setd.at(20).toUInt());
            trkset->setAxisSign(setd.at(21).toUInt());

            emit paramReceiveComplete();
            emit statusMessage(tr("Settings Received"),2000);
        } else {
            emit statusMessage(tr("Error wrong # params"),2000);
        }
    } else if(cmd.left(5) == "$VERS") {
        vers = cmd.mid(5);
        if(!hard.isEmpty()) {
            emit boardDiscovered(this);
            trkset->setHardware(vers,hard);
            startGraph();
        }
    } else if(cmd.left(5) == "$HARD") {
        hard = cmd.mid(5);
        if(!vers.isEmpty()) {
            emit boardDiscovered(this);
            trkset->setHardware(vers,hard);
            startGraph();
        }
    }
}

uint16_t BoardBNO055::escapeCRC(uint16_t crc)
{
    // Characters to escape out
    uint8_t crclow = crc & 0xFF;
    uint8_t crchigh = (crc >> 8) & 0xFF;
    if(crclow == 0x00 ||
       crclow == 0x24)
        crclow ^= 0xFF; //?? why not..
    if(crchigh == 0x00 ||
       crchigh == 0x24)
        crchigh ^= 0xFF; //?? why not..
    return (uint16_t)crclow | ((uint16_t)crchigh << 8);
}

void BoardBNO055::comTimeout()
{

}

void BoardBNO055::startGraph()
{
    serialDataOut += "$PLST\r\n";
    emit serialTxReady();
    graphing = true;
}

void BoardBNO055::stopGraph()
{
    serialDataOut += "$PLST\r\n";
    emit serialTxReady();
    graphing = false;    
}

void BoardBNO055::setDataMode(bool rm)
{
    // Change mode to show offset vs raw unfiltered data
    if(rm)
        serialDataOut += "$GRAW\r\n";
    else
        serialDataOut += "$GOFF\r\n";
    emit serialTxReady();
    rawmode = rm;
}

