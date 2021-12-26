
/*
 * Cliff Blackburn - Nov 20,2020 - Modified code for use with BMO055 + new GUI
 *                               
 */


//-----------------------------------------------------------------------------
// File: Sensors.cpp
// Desc: Implementations sensor board functionality.
//-----------------------------------------------------------------------------
#include "config.h"
#include "Arduino.h"
#include "functions.h"
#include "sensors.h"
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// Variables defined elsewhere
//
extern long channel_value[];

/* BNO055 */
byte bno055_address = 0x28;

Adafruit_BNO055 bno; 
adafruit_bno055_offsets_t calibrationData;
sensors_event_t event; 

// Local variables
byte sensorBuffer[10];       // Buffer for bytes read from sensors
char resetValues = 1;        // Used to reset headtracker/re-center. 

// Final angles for headtracker:
float tiltAngle = 90;       // Tilt angle
float tiltAngleOff = 0;
float tiltAngleLP = 90;     // Tilt angle with low pass FilterSensorData
float lastTiltAngle = 90;   // Used in low pass FilterSensorData.

float rollAngle = 90;        // Roll angle
float rollAngleOff = 0;
float rollAngleLP = 90;     // Roll angle with low pass FilterSensorData
float lastRollAngle = 90;   // Used in low pass FilterSensorData

float panAngle = 90;        // Pan angle
float panAngleOff = 0;
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
int servoPanCenter = 2200;
int servoTiltCenter = 2200;
int servoRollCenter = 2200;
int panMaxPulse = 3200;
int panMinPulse = 1200;
int tiltMaxPulse = 3200;
int tiltMinPulse = 1200;
int rollMaxPulse = 3200;
int rollMinPulse = 1200;
float panFactor = 17;
float tiltFactor = 17;
float rollFactor = 17;
unsigned char servoReverseMask = 0;
unsigned char htChannels[3] = {8, 7, 6}; // pan, tilt, roll
unsigned char axisRemap = Adafruit_BNO055::REMAP_CONFIG_P1;
unsigned char axisSign = Adafruit_BNO055::REMAP_SIGN_P1;
bool graphRaw = false;
long bnoID = 0;
bool foundCalib = false;
bool doCalibrate = false;
sensor_t sensor;

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
void ReadFromI2C(int device, byte address, char bytesToRead)
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

uint8_t sys=0, gyro=0, accel=0, mag = 0;

void trackerOutput()
{  
  bno.getCalibration(&sys, &gyro, &accel, &mag);
  Serial.print("$G");
  if(graphRaw) { // Graph Raw Data
    Serial.print(tiltAngle);
    Serial.print(",");
    Serial.print(rollAngle);
    Serial.print(",");    
    Serial.print(panAngle);
    Serial.print(","); 
  } else {  // Graph offset and filtered data
    Serial.print(tiltAngleLP);
    Serial.print(",");
    Serial.print(rollAngleLP);
    Serial.print(",");    
    Serial.print(panAngleLP);
    Serial.print(",");  
  }
  Serial.print(channel_value[htChannels[0]] / 2 + 400);
  Serial.print(",");    
  Serial.print(channel_value[htChannels[1]] / 2 + 400);
  Serial.print(",");    
  Serial.print(channel_value[htChannels[2]] /2 + 400);
  Serial.print(",");    
  Serial.print(sys, DEC);
  Serial.print(",");    
  Serial.print(gyro, DEC);
  Serial.print(",");    
  Serial.print(accel, DEC);
  Serial.print(",");    
  Serial.println(mag, DEC);  
}

int I2CPresent = 0;

void CheckI2CPresent()
{
  Wire.beginTransmission(0x28);  
  if (Wire.endTransmission() == 0) {
    I2CPresent = 1;
    bno055_address = 0x28;
    bno = Adafruit_BNO055(-1, 0x028 );    
    return;
  } 
  
  Wire.beginTransmission(0x29);  
  if (Wire.endTransmission() == 0) {
    I2CPresent = 1;
    bno055_address = 0x29;
    bno = Adafruit_BNO055(-1, 0x029 );
    return;
  } 
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
        rollStart = rollAngle;         
       
        BEEP_OFF();
    }

    // Offset from reset location
    panAngleOff = panAngle - panStart;
    tiltAngleOff = tiltAngle - tiltStart;
    rollAngleOff = rollAngle - rollStart;

    // Normalize Angle For Pan
    panAngleOff = normalize(panAngleOff,-180,180);
    panAngle = normalize(panAngle,-180,180); 
      
    // Filter the Data
    if (TrackerStarted)
    {
        // All low-pass filters
        
        tiltAngleLP = tiltAngleOff * tiltRollBeta + (1 - tiltRollBeta) * lastTiltAngle;
        lastTiltAngle = tiltAngleLP;
         
        rollAngleLP = rollAngleOff * tiltRollBeta + (1 - tiltRollBeta) * lastRollAngle;
        lastRollAngle = rollAngleLP;
          
        panAngleLP = panAngleOff * panBeta + (1 - panBeta) * lastPanAngle;
        lastPanAngle = panAngleLP;

        // Limit outputs

        float panAngleTemp = (panAngleLP * panInverse * panFactor) + servoPanCenter;
		panAngleTemp = min(max(panAngleTemp, panMinPulse), panMaxPulse);
        channel_value[htChannels[0]] = panAngleTemp;

        float tiltAngleTemp = (tiltAngleLP * tiltInverse * tiltFactor) + servoTiltCenter;
		tiltAngleTemp = min(max(tiltAngleTemp, tiltMinPulse), tiltMaxPulse);
        channel_value[htChannels[1]] = tiltAngleTemp;          

        float rollAngleTemp = (rollAngleLP * rollInverse * rollFactor) + servoRollCenter;
		rollAngleTemp = min(max(rollAngleTemp, rollMinPulse), rollMaxPulse);
		channel_value[htChannels[2]] = rollAngleTemp;          
    }

    float an0ch = ((analogRead(A0) + 988) - 400) *2;
    float an1ch = ((analogRead(A1) + 988) - 400) *2;
    channel_value[0] = an0ch; // **** ANALOG CHANNELS HERE
    channel_value[1] = an1ch;

    // Calibration
    if(sys == 3 && gyro == 3 && accel == 3 && mag == 3 && doCalibrate) {
      StoreBNOCalibration();
      doCalibrate = false;
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
    // Infinate Loop
    while(1) {
      Serial.println("No BNO055 detected... Check your wiring");
      delay(1000);    
    }
  }

  bno.setExtCrystalUse(true);
  bno.setMode((Adafruit_BNO055::adafruit_bno055_opmode_t)0x0C); // 9 Degrees Of Freedom, Fast Mag Cal On
  bno.getSensor(&sensor);
  RemapAxes();  

  // Retreive BNO calibration data if stored in EEPROM
  long bnoID; 
  int eeAddress = BNO_EEPROM_START;
  
  EEPROM.get(eeAddress, bnoID);   
  adafruit_bno055_offsets_t calibrationData;
  
  if (bnoID != sensor.sensor_id) {
      Serial.println("No Calibration Data Stored");
  } else {
      eeAddress += sizeof(long);
      EEPROM.get(eeAddress, calibrationData);
      bno.setSensorOffsets(calibrationData);
      Serial.println("Restored Saved Calibration");
      foundCalib = true;
  }
}

void StoreBNOCalibration() {
  int eeAddress = BNO_EEPROM_START;
  bno.getSensor(&sensor);
  bnoID = sensor.sensor_id; 
  adafruit_bno055_offsets_t newCalib;
  bno.getSensorOffsets(newCalib);
  Serial.println("Storing calibration data to EEPROM");  
  EEPROM.put(eeAddress, bnoID);
  eeAddress += sizeof(long);
  EEPROM.put(eeAddress, newCalib);
  Serial.println("$CALSAV"); // Notify the GUI
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

//--------------------------------------------------------------------------------------
// Func: RemapAxes
// Desc: Allows the tracker board to be re-oriented in multiple positions.
//       Called on startup and on config change.
//       
//--------------------------------------------------------------------------------------

void RemapAxes() 
{
  char cas = axisRemap;

  bno.setAxisRemap((Adafruit_BNO055::adafruit_bno055_axis_remap_config_t)axisRemap);
  bno.setAxisSign((Adafruit_BNO055::adafruit_bno055_axis_remap_sign_t)axisSign);
}


/**************************************************************************/
/*
    Display the raw calibration offset and radius data
    */
/**************************************************************************/

void displaySensorOffsets(const adafruit_bno055_offsets_t &calibData)
{
    Serial.print("Accelerometer: ");
    Serial.print(calibData.accel_offset_x); Serial.print(" ");
    Serial.print(calibData.accel_offset_y); Serial.print(" ");
    Serial.print(calibData.accel_offset_z); Serial.print(" ");

    Serial.print("\nGyro: ");
    Serial.print(calibData.gyro_offset_x); Serial.print(" ");
    Serial.print(calibData.gyro_offset_y); Serial.print(" ");
    Serial.print(calibData.gyro_offset_z); Serial.print(" ");

    Serial.print("\nMag: ");
    Serial.print(calibData.mag_offset_x); Serial.print(" ");
    Serial.print(calibData.mag_offset_y); Serial.print(" ");
    Serial.print(calibData.mag_offset_z); Serial.print(" ");

    Serial.print("\nAccel Radius: ");
    Serial.print(calibData.accel_radius);

    Serial.print("\nMag Radius: ");
    Serial.print(calibData.mag_radius);
}
