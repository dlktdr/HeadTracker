#include <Arduino.h>
#include "sense.h"
#include "main.h"

// Initalize the sensors

void sense_Init()
{  
  /*if(!IMU.begin()) {
    Serial.println("Failed to initalize IMU");    
  }*/
}

// Read all IMU data and do the calculations
// this is run as a Thread, Never Exit

void sense_Thread()
{
  unsigned long microsPerReading, microsPrevious;
  float accx=0,accy=0,accz=0;
  float magx=0,magy=0,magz=0;
  float gyrx=0,gyry=0,gyrz=0;
  float tilt=0,roll=0,pan=0;
  float rolloffset=0, panoffset=0, tiltoffset=0;

  int count=0;
  while(1) {
    // Update Sensors
    /*if(IMU.accelerationAvailable()) {
      IMU.readAcceleration(accx, accy, accz);
    }
    if(IMU.gyroscopeAvailable()) {
      IMU.readGyroscope(gyrx,gyry,gyrz);
      gyrx *= DEG_TO_RAD; // ?? CHECK IF REQUIRED
      gyry *= DEG_TO_RAD;
      gyrz *= DEG_TO_RAD;
    }
    if(IMU.magneticFieldAvailable()) {
      IMU.readMagneticField(magx,magy,magz);
    }
      
    // Period Between Samples
    float deltat = fusion.deltatUpdate();  

    // Do the Calculation
    fusion.MadgwickUpdate(gyrx, gyry, gyrz, accx, accy, accz, magx, magy, magz, deltat);  //else use the magwick, it is slower but more accurate

    pitch = fusion.getPitch();
    roll = fusion.getRoll();
    yaw = fusion.getYaw();
    */

    if(count == 60) {
      /*Serial.print("Accel X "); 
      Serial.print(accx);
      Serial.print(" Y "); 
      Serial.print(accy);
      Serial.print(" Z "); 
      Serial.println(accz);
      
      Serial.print("Mag X "); 
      Serial.print(magx);
      Serial.print(" Y "); 
      Serial.print(magy);
      Serial.print(" Z "); 
      Serial.println(magz);
      
      Serial.print("Gyro X "); 
      Serial.print(gyrx);
      Serial.print(" Y "); 
      Serial.print(gyry);
      Serial.print(" Z "); 
      Serial.println(gyrz);

      Serial.print("Pitch ");
      Serial.print(pitch);
      Serial.print(" Roll ");
      Serial.print(roll);
      Serial.print(" Yaw ");
      Serial.println(yaw);*/

      count = 0;
      }
    count++;

    // Zero button was pressed, 
    if(wasButtonPressed()) {
        rolloffset = roll;
        panoffset = pan;
        tiltoffset = tilt;
    }

    // Roll output
    float rollout = (roll - rolloffset) * trkset.Rll_gain() * (trkset.isRollReversed()? -1.0:1.0);
    rollout = MAX(MIN(rollout,trkset.Rll_max()),trkset.Rll_min());
    ppmout->setChannel(trkset.rollCh(),rollout);
    
    // Pan output, Normalize to +/- 180 Degrees
    float panout = normalize((pan-panoffset),-180,180)  * trkset.Pan_gain() * (trkset.isPanReversed()? -1.0:1.0);
    panout = MAX(MIN(panout,trkset.Pan_max()),trkset.Pan_min());
    ppmout->setChannel(trkset.panCh(),panout);

    // Tilt output
    float tiltout = (tilt - tiltoffset) * trkset.Tlt_gain() * (trkset.isTiltReversed()?-1.0:1.0);
    tiltout = MAX(MIN(tiltout,trkset.Tlt_max()),trkset.Tlt_min());
    ppmout->setChannel(trkset.tiltCh(),tiltout);

    // Wait for next run
    ThisThread::sleep_for(std::chrono::milliseconds(20));
  
  } // END THREAD LOOP
}

// FROM https://stackoverflow.com/questions/1628386/normalise-orientation-between-0-and-360
// Normalizes any number to an arbitrary range 
// by assuming the range wraps around when going below min or above max 
float normalize( const float value, const float start, const float end ) 
{
  const float width       = end - start   ;   // 
  const float offsetValue = value - start ;   // value relative to 0

  return ( offsetValue - ( floor( offsetValue / width ) * width ) ) + start ;
  // + start to reset back to start of original range
}