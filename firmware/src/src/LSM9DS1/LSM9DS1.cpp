/*
  This file is part of the Arduino_LSM9DS1 library.
  This version written by Femme Verbeek, Pijnacker, the Netherlands
  Released to the public domain
  version 2
  Release Date 10 July 2020

  Original notice:

  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA



*/

#include "LSM9DS1.h"

#include <math.h>

#include "defines.h"
#include "log.h"

#define LSM9DS1_ADDRESS 0x6b

#define LSM9DS1_WHO_AM_I 0x0f
#define LSM9DS1_CTRL_REG1_G 0x10
#define LSM9DS1_STATUS_REG 0x17
#define LSM9DS1_OUT_X_G 0x18
#define LSM9DS1_CTRL_REG6_XL 0x20
#define LSM9DS1_CTRL_REG8 0x22
#define LSM9DS1_OUT_X_XL 0x28

// magnetometer
#define LSM9DS1_ADDRESS_M 0x1e

#define LSM9DS1_CTRL_REG1_M 0x20
#define LSM9DS1_CTRL_REG2_M 0x21
#define LSM9DS1_CTRL_REG3_M 0x22
#define LSM9DS1_CTRL_REG4_M 0x23
#define LSM9DS1_STATUS_REG_M 0x27
#define LSM9DS1_OUT_X_L_M 0x28

LSM9DS1Class::LSM9DS1Class() : continuousMode(false) { i2c_dev = NULL; }

LSM9DS1Class::~LSM9DS1Class() {}

int LSM9DS1Class::begin()
{
  i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
  if (!i2c_dev) {
    LOGE("Could not get device binding for I2C");
    return 0;
  }

  storedAccelFS = false;
  storedGyroFS = false;
  storedMagnetFS = false;

  // reset
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG8, 0x05);
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, 0x0c);

  rt_sleep_ms(10);

  if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_WHO_AM_I) != 0x68) {
    end();
    return 0;
  }

  if (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_WHO_AM_I) != 0x3d) {
    end();
    return 0;
  }

  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, 0b10111010);  // 476Hz 57Hz Cut Off
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x70);       // 119 Hz, 4G

  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M,
                0b11111010);  // Temperature comp, ultra HP, 80 Hz, Fast ODR
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, 0x00);  // 4 Gauss
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG3_M, 0x00);  // Continuous conversion mode
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG4_M,
                0b00001100);  // Z-axis operative mode ultra high performance

  measureODRcombined();  // for Accelerometer/Gyro and Magnetometer.
  return 1;
}

void LSM9DS1Class::setContinuousMode()
{
  // Enable FIFO (see docs https://www.st.com/resource/en/datasheet/DM00103319.pdf)
  writeRegister(LSM9DS1_ADDRESS, 0x23, 0x02);
  // Set continuous mode
  writeRegister(LSM9DS1_ADDRESS, 0x2E, 0xC0);

  continuousMode = true;
}

void LSM9DS1Class::setOneShotMode()
{
  // Disable FIFO (see docs https://www.st.com/resource/en/datasheet/DM00103319.pdf)
  writeRegister(LSM9DS1_ADDRESS, 0x23, 0x00);
  // Disable continuous mode
  writeRegister(LSM9DS1_ADDRESS, 0x2E, 0x00);

  continuousMode = false;
}

void LSM9DS1Class::end()
{
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG3_M, 0x03);
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, 0x00);
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x00);
}

//************************************      Acceleration *****************************************

int LSM9DS1Class::readAccel(float& x, float& y,
                            float& z)  // return calibrated data in a unit of choise
{
  if (!readRawAccel(x, y, z)) return 0;
  // See releasenotes   	read =	Unit * Slope * (FS / 32786 * Data - Offset )
  x = accelUnit * accelSlope[0] * (x - accelOffset[0]);
  y = accelUnit * accelSlope[1] * (y - accelOffset[1]);
  z = accelUnit * accelSlope[2] * (z - accelOffset[2]);
  return 1;
}

int LSM9DS1Class::readRawAccel(float& x, float& y, float& z)  // return raw uncalibrated data
{
  int16_t data[3];
  if (!readRegisters(LSM9DS1_ADDRESS, LSM9DS1_OUT_X_XL, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;
    return 0;
  }
  // See releasenotes   	read =	Unit * Slope * (PFS / 32786 * Data - Offset )
  float scale = getAccelFS() / 32768.0;
  x = scale * data[0];
  y = scale * data[1];
  z = scale * data[2];
  return 1;
}

int LSM9DS1Class::accelAvailable()
{
  if (continuousMode) {
    // Read FIFO_SRC. If any of the rightmost 8 bits have a value, there is data.
    if (readRegister(LSM9DS1_ADDRESS, 0x2F) & 63) {
      return 1;
    }
  } else {
    if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_STATUS_REG) & 0x01) {
      return 1;
    }
  }
  return 0;
}

// modified: the void is no longer for translating half calibrated measurements into offsets
// in stead the voids rawAccel()  rawGyro()  and rawMagnet must be used for calibration purposes

void LSM9DS1Class::setAccelOffset(float x, float y, float z)
{
  accelOffset[0] = x / (accelUnit * accelSlope[0]);
  accelOffset[1] = y / (accelUnit * accelSlope[1]);
  accelOffset[2] = z / (accelUnit * accelSlope[2]);
}
// Slope is already dimensionless, so it can be stored as is.
void LSM9DS1Class::setAccelSlope(float x, float y, float z)
{
  if (x == 0) x = 1;  // zero slope not allowed
  if (y == 0) y = 1;
  if (z == 0) z = 1;
  accelSlope[0] = x;
  accelSlope[1] = y;
  accelSlope[2] = z;
}

// range 0: switch off Accel and Gyro write in CTRL_REG6_XL;
// range !=0: switch on Accel: write range in CTRL_REG6_XL ;
//           Operational mode Accel + Gyro: write setting in CTRL_REG1_G, shared ODR
int LSM9DS1Class::setAccelODR(
    uint8_t range)  // Sample Rate 0:off, 1:10Hz, 2:50Hz, 3:119Hz, 4:238Hz, 5:476Hz, 6:952Hz
{
  if (range >= 7) return 0;
  uint8_t setting =
      ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0b00011111) | (range << 5));
  if (writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, setting) == 0) return 0;
  switch (getOperationalMode()) {
    case 0: {
      accelODR = 0;
      gyroODR = 0;
      break;
    }
    case 1: {
      accelODR = measureAccelGyroODR();
      gyroODR = 0;
      break;
    }
    case 2: {
      setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0b00011111) | (range << 5));
      writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, setting);
      accelODR = measureAccelGyroODR();
      gyroODR = accelODR;
    }
  }
  return 1;
}

float LSM9DS1Class::getAccelODR()
{
  return accelODR;
  //	float Ranges[] ={0.0, 10.0, 50.0, 119.0, 238.0, 476.0, 952.0, 0.0 };
  //   uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL)  >> 5;
  //   return Ranges [setting];
}

float LSM9DS1Class::setAccelBW(
    uint8_t range)  // 0,1,2,3 Override autoBandwidth setting see doc.table 67
{
  if (range >= 4) return 0;
  uint8_t RegIs = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0b11111000;
  RegIs = RegIs | 0b00000100 | (range & 0b00000011);
  return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, RegIs);
}

float LSM9DS1Class::getAccelBW()  // Bandwidth setting 0,1,2,3  see documentation table 67
{
  float autoRange[] = {0.0, 408.0, 408.0, 50.0, 105.0, 211.0, 408.0, 0.0};
  float BWXLRange[] = {408.0, 211.0, 105.0, 50.0};
  uint8_t RegIs = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL);
  if (bitRead(RegIs, 2))
    return BWXLRange[RegIs & 0b00000011];
  else
    return autoRange[RegIs >> 5];
}

int LSM9DS1Class::setAccelFS(uint8_t range)  // 0: ±2g ; 1: ±16g ; 2: ±4g ; 3: ±8g
{
  if (range >= 4) return 0;
  lastAccelFS = range;
  storedAccelFS = true;
  range = (range & 0b00000011) << 3;
  uint8_t setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0xE7) | range);
  return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, setting);
}

float LSM9DS1Class::getAccelFS()  // Full scale dimensionless, but its value corresponds to g
{
  float ranges[] = {2.0, 24.0, 4.0, 8.0};  // g
  if (!storedAccelFS) {
    lastAccelFS = (readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0x18) >> 3;
    storedAccelFS = true;
  }
  return ranges[lastAccelFS];
}

//************************************      Gyroscope      *****************************************

int LSM9DS1Class::readGyro(float& x, float& y,
                           float& z)  // return calibrated data in a unit of choise
{
  if (!readRawGyro(x, y, z)) return 0;  // get the register values
  x = gyroUnit * gyroSlope[0] * (x - gyroOffset[0]);
  y = gyroUnit * gyroSlope[1] * (y - gyroOffset[1]);
  z = gyroUnit * gyroSlope[2] * (z - gyroOffset[2]);
  return 1;
}

int LSM9DS1Class::readRawGyro(float& x, float& y,
                              float& z)  // return raw data for calibration purposes
{
  int16_t data[3];
  if (!readRegisters(LSM9DS1_ADDRESS, LSM9DS1_OUT_X_G, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;
    return 0;
  }
  float scale = getGyroFS() / 32768.0;
  x = scale * data[0];
  y = scale * data[1];
  z = scale * data[2];
  return 1;
}
int LSM9DS1Class::gyroAvailable()
{
  if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_STATUS_REG) & 0x02) {
    return 1;
  }
  return 0;
}

// modified: the void is no longer for translating half calibrated measurements into offsets
// Instead the voids rawAccel()  rawGyro()  and rawMagnet must be used for calibration purposes
void LSM9DS1Class::setGyroOffset(float x, float y, float z)
{
  gyroOffset[0] = x;
  gyroOffset[1] = y;
  gyroOffset[2] = z;
}
// Slope is already dimensionless, so it can be stored as is.
void LSM9DS1Class::setGyroSlope(float x, float y, float z)
{
  if (x == 0) x = 1;  // zero slope not allowed
  if (y == 0) y = 1;
  if (z == 0) z = 1;
  gyroSlope[0] = x;
  gyroSlope[1] = y;
  gyroSlope[2] = z;
}

int LSM9DS1Class::getOperationalMode()  // 0=off , 1= Accel only , 2= Gyro +Accel
{
  if ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0b11100000) == 0) return 0;
  if ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0b11100000) == 0)
    return 1;
  else
    return 2;
}

// range ==0 : switch off gyroscope - write 0 in CTRL_REG1_G;
// range !0  : switch on Accel+Gyro mode- write in CTRL_REG6_XL and CTRL_REG1_G;

int LSM9DS1Class::setGyroODR(
    uint8_t range)  // 0:off, 1:10Hz, 2:50Hz, 3:119Hz, 4:238Hz, 5:476Hz, 6:952Hz
{
  if (range >= 7) return 0;
  uint8_t setting =
      ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0b00011111) | (range << 5));
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, setting);
  if (range > 0) {
    setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0b00011111) | (range << 5));
    writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, setting);
  }
  switch (getOperationalMode()) {
    case 0: {
      accelODR = 0;  // off
      gyroODR = 0;
      break;
    }
    case 1: {
      accelODR = measureAccelGyroODR();  // accelerometer only
      gyroODR = 0;
      break;
    }
    case 2: {
      accelODR = measureAccelGyroODR();  // shared ODR
      gyroODR = accelODR;
    }
  }
  return 1;
}

float LSM9DS1Class::getGyroODR()
{
  return gyroODR;
  // float Ranges[] ={0.0, 10.0, 50.0, 119.0, 238.0, 476.0, 952.0, 0.0 };  //Hz
  //   uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G)  >> 5;
  //   return Ranges [setting];   //  used to be  return 119.0F;
}

int LSM9DS1Class::setGyroBW(uint8_t range)
{
  if (range >= 4) return 0;
  range = range & 0b00000011;
  uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0b11111100;
  return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, setting | range);
}

#define ODRrows 8
#define BWcols 4
float BWtable[ODRrows][BWcols] =  // acc to  table 47 dataheet
    {{0, 0, 0, 0},     {0, 0, 0, 0},      {16, 16, 16, 16},  {14, 31, 31, 31},
     {14, 29, 63, 78}, {21, 28, 57, 100}, {33, 40, 58, 100}, {0, 0, 0, 0}};

float LSM9DS1Class::getGyroBW()
{
  uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G);
  uint8_t ODR = setting >> 5;
  uint8_t BW = setting & 0b00000011;
  return BWtable[ODR][BW];
}

int LSM9DS1Class::setGyroFS(uint8_t range)  // (0: 245 dps; 1: 500 dps; 2: 1000  dps; 3: 2000 dps)
{
  if (range >= 4) return 0;
  lastGyroFS = range;
  storedGyroFS = true;
  range = (range & 0b00000011) << 3;
  uint8_t setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0xE7) | range);
  return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, setting);
}

float LSM9DS1Class::getGyroFS()  //   dimensionless, but its value defaults to deg/s
{
  float Ranges[] = {245.0, 500.0, 1000.0, 2000.0};  // dps
  if (!storedGyroFS) {
    lastGyroFS = (readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0x18) >> 3;
    storedGyroFS = true;
  }
  return Ranges[lastGyroFS];
}

//************************************      Magnetic field *****************************************

int LSM9DS1Class::readMagneticField(float& x, float& y,
                                    float& z)  // return calibrated data in a unit of choise
{
  if (!readRawMagnet(x, y, z)) return 0;
  x = magnetUnit * magnetSlope[0] * (x - magnetOffset[0]);
  y = magnetUnit * magnetSlope[1] * (y - magnetOffset[1]);
  z = magnetUnit * magnetSlope[2] * (z - magnetOffset[2]);
  return 1;
}

// return raw data for calibration purposes
int LSM9DS1Class::readRawMagnet(float& x, float& y, float& z)
{
  int16_t data[3];
  if (!readRegisters(LSM9DS1_ADDRESS_M, LSM9DS1_OUT_X_L_M, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;
    return 0;
  }
  float scale = getMagnetFS() / 32768.0;
  x = scale * data[0];
  y = scale * data[1];
  z = scale * data[2];
  return 1;
}

int LSM9DS1Class::magneticFieldAvailable()
{  // return (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_STATUS_REG_M) & 0x08)==0x08;
  if (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_STATUS_REG_M) & 0x08) {
    return 1;
  }
  return 0;
}

// modified: the void is no longer for translating half calibrated measurements into offsets
// Instead the voids rawAccel()  rawGyro()  and rawMagnet must be used for calibration purposes

void LSM9DS1Class::setMagnetOffset(float x, float y, float z)
{
  magnetOffset[0] = x;
  magnetOffset[1] = y;
  magnetOffset[2] = z;
}
// Slope is already dimensionless, so it can be stored as is.
void LSM9DS1Class::setMagnetSlope(float x, float y, float z)
{
  if (x == 0) x = 1;  // zero slope not allowed
  if (y == 0) y = 1;
  if (z == 0) z = 1;
  magnetSlope[0] = x;
  magnetSlope[1] = y;
  magnetSlope[2] = z;
}

int LSM9DS1Class::setMagnetFS(uint8_t range)  // 0=400.0; 1=800.0; 2=1200.0 , 3=1600.0  (µT)
{
  if (range >= 4) return 0;
  lastMagnetFS = range;
  storedMagnetFS = true;
  range = (range & 0b00000011) << 5;
  return writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, range);
}

float LSM9DS1Class::getMagnetFS()  //   dimensionless, but its value defaults to µT
{
  const float Ranges[] = {400.0, 800.0, 1200.0, 1600.0};  //
  if (!storedMagnetFS) {
    lastMagnetFS = readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M) >> 5;
    storedMagnetFS = true;
  }
  return Ranges[lastMagnetFS];
}

int LSM9DS1Class::setMagnetODR(
    uint8_t range)  // range (0..8) = {0.625,1.25,2.5,5,10,20,40,80,400}Hz
{
  if (range >= 16) return 0;
  uint8_t setting = ((range & 0b00000111) << 2) |
                    ((range & 0b00001000) >> 2);  // bit 2..4 see table 111, bit 1 = FAST_ODR
  setting = setting | (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M) & 0b11100001);
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M, setting);
  uint16_t duration =
      1750 / (range + 1);  // 1750,875,666,500,400,333,285,250,222  calculate measuring time
  magnetODR = measureMagnetODR(duration);  // measure the actual ODR value
  return 1;
}

float LSM9DS1Class::getMagnetODR()  // Output {0.625, 1.25, 2.5, 5.0, 10.0, 20.0, 40.0 , 80.0}; //Hz
{
  return magnetODR;  // return previously measured value
}

//************************************      Private functions
//*****************************************

void LSM9DS1Class::measureODRcombined()  // Combined measurement for faster startUp.
{
  float x, y, z;
  unsigned long lastEventTimeA = 0, lastEventTimeM = 0, startA = 0, startM = 0;
  long countA = -3, countM = -2, countiter = 0;  // Extra cycles to compensate for slow startup
  unsigned long start = micros();
  while ((micros() - start) < ODRCalibrationTime) {
    countiter++;
    if (accelAvailable()) {
      lastEventTimeA = micros();
      readAccel(x, y, z);
      countA++;
      if (countA == 0) {
        startA = lastEventTimeA;
        start = lastEventTimeA;
      }
    }
    if (magnetAvailable()) {
      lastEventTimeM = micros();
      readMagnet(x, y, z);
      countM++;
      if (countM == 0) startM = lastEventTimeM;
    }
  }

  accelODR = (1000000.0 * float(countA) / float(lastEventTimeA - startA));
  gyroODR = accelODR;
  magnetODR = (1000000.0 * float(countM) / float(lastEventTimeM - startM));
}

float LSM9DS1Class::measureAccelGyroODR()
{
  if (getOperationalMode() == 0) return 0;
  float x, y, z;  // dummies
  unsigned long lastEventTime = 0, start = micros();
  long count = -3;
  int fifoEna = continuousMode;                    // store FIFO status
  setOneShotMode();                                // switch off FIFO
  while ((micros() - start) < ODRCalibrationTime)  // measure
  {
    if (accelAvailable()) {
      lastEventTime = micros();
      readAccel(x, y, z);
      count++;
      if (count <= 0) start = lastEventTime;
    }
  }

  if (fifoEna) setContinuousMode();
  return (1000000.0 * float(count) / float(lastEventTime - start));
}

float LSM9DS1Class::measureMagnetODR(unsigned long duration)
{
  float x, y, z;  // dummies
  unsigned long lastEventTime = 0, start = micros();
  long count = -2;                       // waste current registervalue and running cycle
  duration *= 1000;                      // switch to micros
  while ((micros() - start) < duration)  // measure
  {
    if (magnetAvailable()) {
      lastEventTime = micros();
      readMagnet(x, y, z);
      count++;
      if (count <= 0) start = lastEventTime;
    }
  }

  return (1000000.0 * float(count) / float(lastEventTime - start));
}

int LSM9DS1Class::readRegister(uint8_t slaveAddress, uint8_t address)
{
  uint8_t rval;
  if (i2c_write_read(i2c_dev, slaveAddress, &address, 1, &rval, 1)) return -1;
  return rval;
}

int LSM9DS1Class::readRegisters(uint8_t slaveAddress, uint8_t address, uint8_t* data, size_t length)
{
  if (i2c_write_read(i2c_dev, slaveAddress, &address, 1, data, length)) return -1;
  return 1;
}

int LSM9DS1Class::writeRegister(uint8_t slaveAddress, uint8_t address, uint8_t value)
{
  uint8_t i2write[2] = {address, value};
  return i2c_write(i2c_dev, i2write, sizeof(i2write), slaveAddress);
}