#include <Arduino.h>
#include "trackersettings.h"
#include "sense.h"
#include "main.h"
#include "dataparser.h"
#include "LSM9DS1/Arduino_LSM9DS1.h"
#include "SensorFusion.h"

SF fusion;

void sense_Init()
{  
  if (!IMU.begin()) {
    serialWriteln("Failed to initalize sensors");
    return;
  }
}

// Read all IMU data and do the calculations,
// This is run as a Thread at Real Time Priority
void sense_Thread()
{
  float accx=0,accy=0,accz=0;
  float magx=0,magy=0,magz=0;
  float gyrx=0,gyry=0,gyrz=0;
  float tilt=0,roll=0,pan=0;
  float rolloffset=0, panoffset=0, tiltoffset=0;
  
  while(1) {
    if(IMU.accelerationAvailable()) {
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

    roll = fusion.getPitch();
    tilt = fusion.getRoll();
    pan = fusion.getYaw();

    // Zero button was pressed, 
    if(wasButtonPressed()) {
        rolloffset = roll;
        panoffset = pan;
        tiltoffset = tilt;
    }    

    // Tilt output
    float tiltout = (tilt - tiltoffset) * trkset.Tlt_gain() * (trkset.isTiltReversed()?-1.0:1.0);
    uint16_t tiltout_ui = tiltout + trkset.Tlt_cnt();
    tiltout_ui = MAX(MIN(tiltout_ui,trkset.Tlt_max()),trkset.Tlt_min());
    ppmout->setChannel(trkset.tiltCh(),tiltout_ui);

    // Roll output
    float rollout = (roll - rolloffset) * trkset.Rll_gain() * (trkset.isRollReversed()? -1.0:1.0);
    uint16_t rollout_ui = rollout + trkset.Rll_cnt();
    rollout_ui = MAX(MIN(rollout_ui,trkset.Rll_max()),trkset.Rll_min()) ;
    ppmout->setChannel(trkset.rollCh(),rollout_ui);
    
    // Pan output, Normalize to +/- 180 Degrees
    float panout = normalize((pan-panoffset),-180,180)  * trkset.Pan_gain() * (trkset.isPanReversed()? -1.0:1.0);    
    uint16_t panout_ui = panout + trkset.Pan_cnt();
    panout_ui = MAX(MIN(panout_ui,trkset.Pan_max()),trkset.Pan_min());
    ppmout->setChannel(trkset.panCh(),panout_ui);

    // Update the settings
    // Both data and sensor threads will use this data. If data thread has it locked skip this reading.
    if(dataMutex.trylock()) {
        trkset.setRawAccel(accx,accy,accz);
        trkset.setRawGyro(gyrx,gyry,gyrz);
        trkset.setRawMag(magx,magy,magz);
        trkset.setRawOrient(tilt,roll,pan);
        trkset.setOffOrient(tilt-tiltoffset,roll-rolloffset, normalize(pan-panoffset,-180,180));
        trkset.setPPMOut(tiltout_ui,rollout_ui,panout_ui);
        dataMutex.unlock();
    }

    // Wait for next run
    ThisThread::sleep_for(std::chrono::milliseconds(10)); // ~100Hz

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