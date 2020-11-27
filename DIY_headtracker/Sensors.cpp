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
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>


// Variables defined elsewhere
//
extern long channel_value[];

/* BNO055 */
Adafruit_BNO055 bno = Adafruit_BNO055(-1, BNO055_ADDRESS ); 
adafruit_bno055_offsets_t calibrationData;
Adafruit_BNO055::adafruit_bno055_axis_remap_config_t rmc = Adafruit_BNO055::REMAP_CONFIG_P1;
Adafruit_BNO055::adafruit_bno055_axis_remap_sign_t rms = Adafruit_BNO055::REMAP_SIGN_P1;

sensors_event_t event; 

// Local variables
//
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
unsigned char axisRemap = 0;
bool graphRaw = false;

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
  uint8_t sys, gyro, accel, mag = 0;
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
  Serial.print(channel_value[htChannels[0]]);
  Serial.print(",");    
  Serial.print(channel_value[htChannels[1]]);
  Serial.print(",");    
  Serial.print(channel_value[htChannels[2]]);
  Serial.print(",");    
  Serial.print(sys, DEC);
  Serial.print(",");    
  Serial.print(gyro, DEC);
  Serial.print(",");    
  Serial.print(accel, DEC);
  Serial.print(",");    
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
        if(panAngleTemp < panMinPulse)
          panAngleTemp = panMinPulse;
        if(panAngleTemp > panMaxPulse)
          panAngleTemp = panMaxPulse;
        channel_value[htChannels[0]] = panAngleTemp;          

        float tiltAngleTemp = (tiltAngleLP * tiltInverse * tiltFactor) + servoTiltCenter;
        if(tiltAngleTemp < tiltMinPulse)
          tiltAngleTemp = tiltMinPulse;
        if(tiltAngleTemp > tiltMaxPulse)
          tiltAngleTemp = tiltMaxPulse;
        channel_value[htChannels[1]] = tiltAngleTemp;          

        float rollAngleTemp = (rollAngleLP * rollInverse * rollFactor) + servoRollCenter;
        if(rollAngleTemp < rollMinPulse)
          rollAngleTemp = rollMinPulse;
        if(rollAngleTemp > rollMaxPulse)
          rollAngleTemp = rollMaxPulse;
        channel_value[htChannels[2]] = rollAngleTemp;          
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
  //bno.setMode(0X0C); // 9 Degrees Of Freedom, Fast Mag Cal On

  sensor_t sensor;
  bno.getSensor(&sensor);
  // Display some infor on the Sensor, from BNO055 Example
  Serial.println("------------------------------------");
  Serial.print("Sensor:       "); Serial.println(sensor.name);
  Serial.print("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print("Max Value:    "); Serial.print(sensor.max_value); Serial.println("");
  Serial.print("Min Value:    "); Serial.print(sensor.min_value); Serial.println("");
  Serial.print("Resolution:   "); Serial.print(sensor.resolution); Serial.println("");
  Serial.println("------------------------------------");  

  // Set Axes Orientation
  RemapAxes();  
  Serial.print("Axis Remap: "); Serial.print(axisRemap); Serial.print(" 0x"); Serial.println((char)rmc,HEX);
  Serial.print("Axis Sign:    0x"); Serial.println((char)rms,HEX);
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
// Desc: Allows the tracker board to be re-oriented in multiple positions. 8 Choices allowed here. See BNO055 Datasheet Pg. 25
//       Called on startup and on config change.
//       Possible Choices are, Board Flat, Sensor UP (Default) X=X Y=Y Z=Z 0x24 SIGN=0x00
//--------------------------------------------------------------------------------------

#define AXIS_X 0x00
#define AXIS_Y 0x01
#define AXIS_Z 0x02

#define AXES_MAP(XX,YY,ZZ) (ZZ<<4|YY<<2|XX)

#define X_REV 0x04
#define Y_REV 0x02
#define Z_REV 0x01

void RemapAxes() 
{
  rmc = AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z);
  rms = 0;
  char cas = axisRemap;
  switch(cas) {
    // Board Up, Tilt towards long edge : Default
    case 0:     
        rmc = AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z); // Board Flat, Sensor Facing Up
        rms = 0;
        break;          
    case 1:
        rmc = AXES_MAP(AXIS_Y,AXIS_X,AXIS_Z); // Board Flat, Sensor Facing up, Rotate 90
        rms = Y_REV;
        break;    
    case 2: 
        rmc = AXES_MAP(AXIS_X,AXIS_Z,AXIS_Y); // Board Vertical, Usb down, Sensor Facing right
        rms = Z_REV;
        break;
    // Board Facking Down, Tilt towards long edge
    case 3: 
        rmc = AXES_MAP(AXIS_Y,AXIS_Z,AXIS_X); // Board Vertical, Usb Left Sesnsor Facing Back 
        rms = X_REV|Z_REV;
        break;
    case 4: 
        rmc = AXES_MAP(AXIS_X,AXIS_Z,AXIS_Y); // Board Vertical, Usb Down, Sensor Forward
        rms = Z_REV;
        break;
    case 5: 
        rmc = AXES_MAP(AXIS_Z,AXIS_Y,AXIS_X); // TBD
        rms = Z_REV;
        break;
   case 6: 
        rmc = AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z); // TBD 
        rms = Y_REV;
        break;
   case 7: 
        rmc = AXES_MAP(AXIS_X,AXIS_Y,AXIS_Z); // TBD 
        rms = Y_REV|X_REV;
        break;        
  }


  bno.setAxisRemap(rmc);
  bno.setAxisSign(rms);
  Serial.print("Orient Mapping #"); Serial.print(axisRemap); Serial.print(" Set To: "); Serial.print(rmc, HEX); Serial.print(" Signs: "); Serial.print(rms, HEX); Serial.println("");
     
/*  rmc = Adafruit_BNO055::REMAP_CONFIG_P1;
  rms = Adafruit_BNO055::REMAP_SIGN_P1;
    
  if(axisRemap == 0 || // 0x21
     axisRemap == 3 ||
     axisRemap == 5 ||
     axisRemap == 6) 
      rmc = Adafruit_BNO055::REMAP_CONFIG_P0; // 0x24
  else
      rmc = Adafruit_BNO055::REMAP_CONFIG_P1;

  if(axisRemap < 7)
    rms = (Adafruit_BNO055::adafruit_bno055_axis_remap_sign_t)mapcodes[axisRemap];
*/
  //bno.setAxisSign(rms);
  //bno.setAxisRemap(rmc);
}
