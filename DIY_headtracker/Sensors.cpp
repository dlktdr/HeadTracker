/*
 * Cliff Blackburn - Nov 20,2020 - Modified code for use with alternate sensors including BMO055 Sensor + QMC5883L
 *                               - Auto discovery on powerup via address scan.
 *                               - BMO055 uses alternate calibration
 */
//-----------------------------------------------------------------------------
// File: Sensors.cpp
// Desc: Implementations sensor board functionality.
//-----------------------------------------------------------------------------
#include "config.h"
#include "Arduino.h"
#include "functions.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

/*
Reference for basic sensor/IMU understanding/calculation:
http://www.starlino.com/imu_guide.html
http://www.sparkfun.com/datasheets/Sensors/Magneto/Tilt%20Compensated%20Compass.pdf
https://www.loveelectronics.co.uk/Tutorials/13/tilt-compensated-compass-arduino-tutorial
http://www.pololu.com/file/download/LSM303DLH-compass-app-note.pdf?file_id=0J434
*/

// BNO055 Can use either 0x28 or 0x29 depending on wiring of the ADR pin.
#define BNO055_ADDRESS 0x29

// Variables defined elsewhere
//
extern long channel_value[];

/* BNO055 */
Adafruit_BNO055 bno = Adafruit_BNO055(-1,0x29); // **** DEFAULT BNO055 address, 0x028 also Possible ****
adafruit_bno055_offsets_t calibrationData;
sensors_event_t event; 

// Local variables
//
byte sensorBuffer[10];       // Buffer for bytes read from sensors
char resetValues = 1;        // Used to reset headtracker/re-center. 


// Final angles for headtracker:
float tiltAngle = 90;       // Tilt angle
float tiltAngleLP = 90;     // Tilt angle with low pass FilterSensorData
float lastTiltAngle = 90;   // Used in low pass FilterSensorData.

float rollAngle = 90;        // Roll angle
float rollAngleLP = 90;     // Roll angle with low pass FilterSensorData
float lastRollAngle = 90;   // Used in low pass FilterSensorData

float panAngle = 90;        // Pan angle
float panAngleLP = 90;      // Pan angle with low pass FilterSensorData
float lastPanAngle = 90;    // Used in low pass FilterSensorData

// Start values - center position for head tracker
float tiltStart = 0;
float panStart = 0;
float rollStart = 0;

char TrackerStarted = 0;

// Servo reversing
char tiltInverse = -1;
char rollInverse = -1;
char panInverse = -1;

// Settings
//
float tiltRollBeta = 0.75;
float panBeta = 0.75;
float gyroWeightTiltRoll = 0.98;
float GyroWeightPan = 0.98;
int servoPanCenter = 2100;
int servoTiltCenter = 2100;
int servoRollCenter = 2100;
int panMaxPulse = 1150;
int panMinPulse = 1150;
int tiltMaxPulse = 1150;
int tiltMinPulse = 1150;
int rollMaxPulse = 1150;
int rollMinPulse = 1150;
float panFactor = 17;
float tiltFactor = 17;
float rollFactor = 17;
unsigned char servoReverseMask = 0;
unsigned char htChannels[3] = {8, 7, 6}; // pan, tilt, roll
//
// End settings


// Function used to write to I2C:
void WriteToI2C(int device, byte address, byte val)
{
    Wire.beginTransmission(device);
    Wire.write(address);
    Wire.write(val);
    Wire.endTransmission();
}

// Function to read from I2C
void ReadFromI2C(int device, char address, char bytesToRead)
{
    Wire.beginTransmission(device);
    Wire.write(address);
    Wire.endTransmission();
  
    Wire.beginTransmission(device);
    Wire.requestFrom(device, bytesToRead);
   
    char i = 0;   
    while ( Wire.available() )
    {
        sensorBuffer[i++] = Wire.read();
    }   
    Wire.endTransmission();
}

void trackerOutput()
{
  Serial.print(tiltAngleLP );
  Serial.print(",");
  Serial.print(rollAngleLP );
  Serial.print(",");    
  Serial.println(panAngleLP);

  /* Also send calibration data for each sensor. */
  uint8_t sys, gyro, accel, mag = 0;
  bno.getCalibration(&sys, &gyro, &accel, &mag);
  Serial.print(F("Calibration: "));
  Serial.print(sys, DEC);
  Serial.print(F(" "));
  Serial.print(gyro, DEC);
  Serial.print(F(" "));
  Serial.print(accel, DEC);
  Serial.print(F(" "));
  Serial.println(mag, DEC);

  
}

//--------------------------------------------------------------------------------------
// Func: UpdateSensors
// Desc: Retrieves the sensor data from the sensor board via I2C.
//--------------------------------------------------------------------------------------
void UpdateSensors()
{    
    bno.getEvent(&event);  
}


//--------------------------------------------------------------------------------------
// Func: Filter
// Desc: Filters / merges sensor data. 
//--------------------------------------------------------------------------------------

// FROM https://stackoverflow.com/questions/1628386/normalise-orientation-between-0-and-360
// Normalizes any number to an arbitrary range 
// by assuming the range wraps around when going below min or above max 
double normalize( const float value, const float start, const float end ) 
{
  const float width       = end - start   ;   // 
  const float offsetValue = value - start ;   // value relative to 0

  return ( offsetValue - ( floor( offsetValue / width ) * width ) ) + start ;
  // + start to reset back to start of original range
}

void FilterSensorData()
{
    int temp = 0;

    // Get Orientation from BNO055
    rollAngle = event.orientation.z;
    panAngle = event.orientation.x;
    tiltAngle  = event.orientation.y;

    // Used to set initial values. 
    if (resetValues == 1)
    {
        BEEP_ON();
        resetValues = 0; 
      
        tiltStart = tiltAngle;
        
        panStart = panAngle;
        Serial.print("PANANGLE");  Serial.println(panAngle);
        
        rollStart = rollAngle;         
        
        BEEP_OFF();
    }

    // Offset from reset location
    panAngle -= panStart;
    tiltAngle -= tiltStart;
    rollAngle -= rollStart;

    // Normalize Angle ON Pan
    panAngle = normalize(panAngle,-180,180);
      
    // Filter the Data
    if (TrackerStarted)
    {
        // All low-pass filters
        tiltAngleLP = tiltAngle * tiltRollBeta + (1 - tiltRollBeta) * lastTiltAngle;
        lastTiltAngle = tiltAngleLP;
  
        rollAngleLP = rollAngle * tiltRollBeta + (1 - tiltRollBeta) * lastRollAngle;
        lastRollAngle = rollAngleLP;

        panAngleLP = panAngle * panBeta + (1 - panBeta) * lastPanAngle;
        lastPanAngle = panAngleLP;

        float panAngleTemp = (panAngleLP) * panInverse * panFactor;
        if ( (panAngleTemp > -panMinPulse) && (panAngleTemp < panMaxPulse) )
        {
            temp = servoPanCenter + panAngleTemp;
            channel_value[htChannels[0]] = (int)temp;
        }    

        float tiltAngleTemp = (tiltAngleLP) * tiltInverse * tiltFactor;
        if ( (tiltAngleTemp > -tiltMinPulse) && (tiltAngleTemp < tiltMaxPulse) )
        {
            temp = servoTiltCenter + tiltAngleTemp;
            channel_value[htChannels[1]] = temp;
        }   

        float rollAngleTemp = (rollAngleLP) * rollInverse * rollFactor;
        if ( (rollAngleTemp > -rollMinPulse) && (rollAngleTemp < rollMaxPulse) )
        {
            temp = servoRollCenter + rollAngleTemp;
            channel_value[htChannels[2]] = temp;
        }
    }
}

//--------------------------------------------------------------------------------------
// Func: InitSensors
// Desc: Initializes the sensor board sensors.
//--------------------------------------------------------------------------------------
void InitSensors()
{
  /* Initialise the sensor */
  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    // Infinate Loop
    while(1);
  }

  bno.setExtCrystalUse(true);

  sensor_t sensor;
  bno.getSensor(&sensor);
  // Display some infor on the Sensor, from BNO055 Example
  Serial.println("------------------------------------");
  Serial.print("Sensor:       "); Serial.println(sensor.name);
  Serial.print("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" xxx");
  Serial.print("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" xxx");
  Serial.print("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" xxx");
  Serial.println("------------------------------------");

  
}

//--------------------------------------------------------------------------------------
// Func: ResetCenter
// Desc: Utility for resetting tracker center. This is only called during tracker
//       startup. Button press resets are handled during filtering. (Needs refactor)
//--------------------------------------------------------------------------------------
void ResetCenter()
{
    resetValues = 1; 
    TrackerStarted = 1;
}
