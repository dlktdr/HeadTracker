#include <Arduino.h>
#include "trackersettings.h"
#include "sense.h"
#include "main.h"
#include "dataparser.h"
#include "LSM9DS1/Arduino_LSM9DS1.h"
#include "SensorFusion.h"
#include "serial.h"
#include "Wire.h"
#include "NXPFusion/Adafruit_AHRS_NXPFusion.h"

Adafruit_NXPSensorFusion nxpfilter;

void sense_Init()
{ 
  // I2C High Speed  
  if (!IMU.begin()) {
    serialWriteln("Failed to initalize sensors");
    return;
  }  
  // Increase Clock Speed, save some CPU, pretty sure it's using blocking
  Wire1.setClock(400000);  

  nxpfilter.begin(96); // Frequency descovered by oscilliscope

  IMU.setGyroFS(1);
  IMU.setGyroODR(3);
}



// Read all IMU data and do the calculations,
// This is run as a Thread at Real Time Priority
void sense_Thread()
{
  float raccx=0,raccy=0,raccz=0;
  float rmagx=0,rmagy=0,rmagz=0;
  float rgyrx=0,rgyry=0,rgyrz=0;
  
  float accx=0,accy=0,accz=0;
  float magx=0,magy=0,magz=0;
  float gyrx=0,gyry=0,gyrz=0;
  float tilt=0,roll=0,pan=0;
  float rolloffset=0, panoffset=0, tiltoffset=0;
  float magxoff=0, magyoff=0, magzoff=0;
  float accxoff=0, accyoff=0, acczoff=0;
  float gyrxoff=0, gyryoff=0, gyrzoff=0;
  SF fusion;

  
  int counter=0;

  while(1) {
    digitalWrite(D8,HIGH);
    
    // Accelerometer
    if(IMU.accelerationAvailable()) {
      IMU.readRawAccel(raccx, raccy, raccz);
      trkset.accOffset(accxoff,accyoff,acczoff);
      accx = raccx - accxoff;
      accy = raccy - accyoff;
      accz = raccz - acczoff;
    }

    // Gyrometer
    if(IMU.gyroscopeAvailable()) {
      IMU.readRawGyro(rgyrx,rgyry,rgyrz);
      trkset.gyroOffset(gyrxoff,gyryoff,gyrzoff);
    //  gyrx *= DEG_TO_RAD;
    //  gyry *= DEG_TO_RAD;
   //   gyrz *= DEG_TO_RAD;
      gyrx = rgyrx - gyrxoff; 
      gyry = rgyrx - gyryoff; 
      gyrz = rgyrx - gyrzoff; 
      gyrx *= -1;
      gyry *= -1;
      gyrz *= -1;

    }

    // Magnetometer
    if(IMU.magneticFieldAvailable()) {
      IMU.readRawMagnet(rmagx,rmagy,rmagz);
      // On first read set the min/max values to this reading
      // Get Offsets and Apply them
      trkset.magOffset(magxoff,magyoff,magzoff);
            
      magx = rmagx-magxoff;//mxr;
      magy = rmagy-magyoff;//myr;
      magz = rmagz-magzoff;///mzr;
    }

    // Period Between Samples
/*    float deltat = fusion.deltatUpdate();

    // Do the Calculation
    //fusion.MahonyUpdate(gyrx, gyry, gyrz, accx, accy, accz, magx, magy, magz, deltat);  //else use the magwick, it is slower but more accurate
    fusion.MadgwickUpdate(gyrx, gyry, gyrz, accx, accy, accz, magx, magy, magz, deltat);  //else use the magwick, it is slower but more accurate
    
    roll = fusion.getPitch();
    tilt = fusion.getRoll();
    pan = fusion.getYaw();*/

    // NXP 
    nxpfilter.update(gyrx, gyry, gyrz, accx, accy, accz, magx, magy, magz);
    roll = nxpfilter.getPitch();
    tilt = nxpfilter.getRoll();
    pan = nxpfilter.getYaw();

    // Zero button was pressed, adjust all values to zero
    if(wasButtonPressed()) {
        rolloffset = roll;
        panoffset = pan;
        tiltoffset = tilt;
    }

    // Tilt output
    float tiltout = (tilt - tiltoffset) * trkset.Tlt_gain()/10 * (trkset.isTiltReversed()?-1.0:1.0);
    uint16_t tiltout_ui = tiltout + trkset.Tlt_cnt();
    tiltout_ui = MAX(MIN(tiltout_ui,trkset.Tlt_max()),trkset.Tlt_min());
    ppmout->setChannel(trkset.tiltCh(),tiltout_ui);

    // Roll output
    float rollout = (roll - rolloffset) * trkset.Rll_gain() /10 * (trkset.isRollReversed()? -1.0:1.0);
    uint16_t rollout_ui = rollout + trkset.Rll_cnt();
    rollout_ui = MAX(MIN(rollout_ui,trkset.Rll_max()),trkset.Rll_min()) ;
    ppmout->setChannel(trkset.rollCh(),rollout_ui);
    
    // Pan output, Normalize to +/- 180 Degrees
    float panout = normalize((pan-panoffset),-180,180)  * trkset.Pan_gain()/10 * (trkset.isPanReversed()? -1.0:1.0);    
    uint16_t panout_ui = panout + trkset.Pan_cnt();
    panout_ui = MAX(MIN(panout_ui,trkset.Pan_max()),trkset.Pan_min());
    ppmout->setChannel(trkset.panCh(),panout_ui);


     // 'Raw' values to match expectation of MOtionCal
    /*serialWrite("Raw:");
    serialWrite(int(accx*8192/9.8)); serialWrite(",");
    serialWrite(int(accy*8192/9.8)); serialWrite(",");
    serialWrite(int(accz*8192/9.8)); serialWrite(",");
    serialWrite(int(gyrx*16)); serialWrite(",");
    serialWrite(int(gyry*16)); serialWrite(",");
    serialWrite(int(gyrz*16)); serialWrite(",");
    serialWrite(int(magx*10)); serialWrite(",");
    serialWrite(int(magy*10)); serialWrite(",");
    serialWrite(int(magz*10)); serialWriteln("");

    // unified data
    serialWrite("Uni:");
    serialWriteF(accx); serialWrite(",");
    serialWriteF(accy); serialWrite(",");
    serialWriteF(accz); serialWrite(",");
    serialWriteF(gyrx); serialWrite(",");
    serialWriteF(gyry); serialWrite(",");
    serialWriteF(gyrz); serialWrite(",");
    serialWriteF(magx); serialWrite(",");
    serialWriteF(magy); serialWrite(",");
    serialWriteF(magz); serialWriteln("");*/

    // Update the settings
    // Both data and sensor threads will use this data. If data thread has it locked skip this reading.
    if(dataMutex.trylock()) {
        trkset.setRawAccel(raccx,raccy,raccz);
        trkset.setRawGyro(rgyrx,rgyry,rgyrz);
        trkset.setRawMag(rmagx,rmagy,rmagz);
        trkset.setRawOrient(tilt,roll,pan);
        trkset.setOffOrient(tilt-tiltoffset,roll-rolloffset, normalize(pan-panoffset,-180,180));
        trkset.setPPMOut(tiltout_ui,rollout_ui,panout_ui);
        dataMutex.unlock();
    }

    
    digitalWrite(D8,LOW); // Used to measure actual time to execute this on a scope

    // Wait for next run

    // Takes apx 2.5ms to execute this. 99% used in I2C reads..
    // Calculations are using 115us of CPU time.
    ThisThread::sleep_for(std::chrono::milliseconds(8)); // apx 60hz update rate

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