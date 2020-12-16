//-----------------------------------------------------------------------------
// File: Sensors.h
// Desc: Declares sensor-related functions for the project.
//-----------------------------------------------------------------------------
#ifndef sensors_h
#define sensors_h

#include "Arduino.h"

// Axis Mapping
#define AXIS_X 0x00
#define AXIS_Y 0x01
#define AXIS_Z 0x02
#define AXES_MAP(XX,YY,ZZ) (ZZ<<4|YY<<2|XX)
#define X_REV 0x04
#define Y_REV 0x02
#define Z_REV 0x01

void InitSensors();
void WriteToI2C(int device, byte address, byte val);
void ReadFromI2C(int device, byte address, char bytesToRead);
void UpdateSensors();
void GyroCalc();
void AccelCalc();
void MagCalc();

void SetGyroOffset();
void trackerOutput();
void FilterSensorData();
void ResetCenter();
void SensorInfoPrint();
void CheckI2CPresent();

void RemapAxes();

#endif // sensors_h
