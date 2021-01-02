#include <Arduino.h>
#include <Arduino_APDS9960.h>

#include "trackersettings.h"
#include "sense.h"
#include "mbed.h"
#include "main.h"
#include "dataparser.h"
#include "LSM9DS1/Arduino_LSM9DS1.h"
#include "SensorFusion.h"
#include "serial.h"
#include "Wire.h"
#include "NXPFusion/Adafruit_AHRS_NXPFusion.h"


// Pick one
#define NXP_FILTER 
//#define MADGWICK 
//#define MAHONY   

#ifdef NXP_FILTER
Adafruit_NXPSensorFusion nxpfilter;
#endif

bool blesenseboard = true;

int sense_Init()
{ 
    // I2C High Speed  
    if (!IMU.begin()) {
        serialWriteln("Failed to initalize sensors");
        return -1;
    }  
    // Increase Clock Speed, save some CPU, pretty sure it's using blocking in lsmlib
    Wire1.setClock(400000);  

#ifdef NXP_FILTER
    nxpfilter.begin(125); // Frequency discovered by oscilliscope
#else
    IMU.setGyroFS(3); // 2000dps gyro
    IMU.setGyroODR(1);
#endif

    if(!APDS.begin()) 
        blesenseboard = false;
    
    return 0;        
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
    float rotatex=0,rotatety=0,rotatez=0;

    PpmOut *ppmout;
    PpmIn *ppmin;
    uint16_t ppmchans[MAX_PPM_CHANNELS];
    SF fusion;
    Timer runt;
    
    int counter=0;
    int lastgesture = -1;

    while(1) {

        if(pauseForEEPROM) {
        ThisThread::sleep_for(std::chrono::milliseconds(100)); 
        continue;
        }

        // Used to measure how long this took to adjust sleep timer
        runt.reset();
        runt.start();
        
        digitalWrite(A0,HIGH);

        // Run this first to keep most accurate timing    
    #ifdef NXP_FILTER
        // NXP 
        nxpfilter.update(gyrx, gyry, gyrz, accx*9.807, accy*9.807, accz*9.807, magx, magy, magz);
        roll = nxpfilter.getPitch();
        tilt = nxpfilter.getRoll();
        pan = nxpfilter.getYaw();
    #else 
        // Period Between Samples
        float deltat = fusion.deltatUpdate();
    #ifdef MADGWICK 
        fusion.MadgwickUpdate(gyrx * DEG_TO_RAD, gyry * DEG_TO_RAD, gyrz * DEG_TO_RAD, 
                            accx*9.807, accy*9.807, accz*9.807, 
                            magx, magy, magz, 
        deltat);  
    #elif defined(MAHONY)
        fusion.MahonyUpdate(gyrx, gyry, gyrz, accx, accy, accz, magx, magy, magz, deltat);  //else use the magwick, it is slower but more accurate
    #endif    

        roll = fusion.getPitch();
        tilt = fusion.getRoll();
        pan = fusion.getYaw();
    #endif

        // Reset Center on Wave
        if(blesenseboard) {
            if (trkset.resetOnWave() && APDS.gestureAvailable()) {
                // a gesture was detected, read and print to serial monitor                
                int gesture = APDS.readGesture();               
                if(lastgesture == GESTURE_DOWN && 
                   gesture == GESTURE_UP)
                    pressButton();
                else if(lastgesture == GESTURE_UP && 
                        gesture == GESTURE_DOWN)
                    pressButton();
                else if(lastgesture == GESTURE_LEFT && 
                        gesture == GESTURE_RIGHT)
                    pressButton();
                else if(lastgesture == GESTURE_RIGHT && 
                        gesture == GESTURE_LEFT)
                    pressButton();
                lastgesture = gesture;
            }
        }

        // Zero button was pressed, adjust all values to zero
        if(wasButtonPressed()) {
            rolloffset = roll;
            panoffset = pan;
            tiltoffset = tilt;
        }
        
        // Tilt output
        float tiltout = (tilt - tiltoffset) * trkset.Tlt_gain() * (trkset.isTiltReversed()?-1.0:1.0);
        uint16_t tiltout_ui = tiltout + trkset.Tlt_cnt();
        tiltout_ui = MAX(MIN(tiltout_ui,trkset.Tlt_max()),trkset.Tlt_min());
        
        // Roll output
        float rollout = (roll - rolloffset) * trkset.Rll_gain() * (trkset.isRollReversed()? -1.0:1.0);
        uint16_t rollout_ui = rollout + trkset.Rll_cnt();
        rollout_ui = MAX(MIN(rollout_ui,trkset.Rll_max()),trkset.Rll_min()) ;
        
        // Pan output, Normalize to +/- 180 Degrees
        float panout = normalize((pan-panoffset),-180,180)  * trkset.Pan_gain() * (trkset.isPanReversed()? -1.0:1.0);    
        uint16_t panout_ui = panout + trkset.Pan_cnt();
        panout_ui = MAX(MIN(panout_ui,trkset.Pan_max()),trkset.Pan_min());

        // Reset all PPM Channels to Center
        for(int i=0;i<MAX_PPM_CHANNELS;i++)
            ppmchans[i] = TrackerSettings::DEF_CENTER;

        // Read all PPM inputs, if enabled
        ppmout = trkset.getPpmOut();
        if(ppmout != nullptr) {    
            // Read All PPM Inputs
            // *** TODO
        }

        // Set the PPM Outputs, if output enabled
        ppmout = trkset.getPpmOut();
        if(ppmout != nullptr) {
            ppmchans[trkset.tiltCh()] = tiltout_ui;
            ppmchans[trkset.rollCh()] = rollout_ui;
            ppmchans[trkset.panCh()] = panout_ui;
            // Send them all
            for(int i=0;i<MAX_PPM_CHANNELS;i++) {
                ppmout->setChannel(i,ppmchans[i]);
            }
        }
        

        // ---------------------- Get new data
    //********************************
    // THIS CODE NEEDS TO BE FASTER DOWN HERE

        if(++counter == SENSEUPDATE) { // Run Filter 5 more often than measurements
        counter = 0;
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
            gyrx = rgyrx - gyrxoff; 
            gyry = rgyry - gyryoff; 
            gyrz = rgyrz - gyrzoff; 
            gyrx *= -1;
            gyry *= -1;
            //gyrz *= -1;
        }

        // Magnetometer
        if(IMU.magneticFieldAvailable()) {
            IMU.readRawMagnet(rmagx,rmagy,rmagz);
            // On first read set the min/max values to this reading
            // Get Offsets and Apply them
            trkset.magOffset(magxoff,magyoff,magzoff);              

            // Calibrate Hard Iron Offsets
            magx = rmagx - magxoff; 
            magy = rmagy - magyoff;
            magz = rmagz - magzoff;       

            // Calibrate soft iron offsets 
            float magsioff[9];
            trkset.magSiOffset(magsioff);
            magx = magx * magsioff[0] + magy * magsioff[1] + magz *magsioff[2];
            magy = magx * magsioff[3] + magy * magsioff[4] + magz *magsioff[5];
            magz = magx * magsioff[6] + magy * magsioff[7] + magz *magsioff[8];

        /* static int o=0;
            if(o++ > 50) {
            o=0;          
            serialWrite("MagX:");
            serialWrite(magx);
            serialWrite(" Y:");
            serialWrite(magy);
            serialWrite(" Z:");
            serialWrite(magz);
            serialWriteln("");

            serialWrite("CalX:");
            serialWrite(magxoff);
            serialWrite(" Y:");
            serialWrite(magyoff);
            serialWrite(" Z:");
            serialWrite(magzoff);
            serialWriteln("");
            }*/
        }
        }

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
        
        digitalWrite(A0,LOW); // Pin for timing check

        // Use this time to determine how long to sleep for
        runt.stop();
        using namespace std::chrono;
        int micros = duration_cast<microseconds>(runt.elapsed_time()).count();
        int sleepyt = (SLEEPTIME - micros); // Ideally 5ms period

        // Don't sleep for too long it something wrong above
        if(sleepyt < 5)
        sleepyt = 5;    

        // **** Not super accurate being only in ms values
        // Not sure why but +1 works out on time
        ThisThread::sleep_for(std::chrono::milliseconds(sleepyt/1000+1)); 

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