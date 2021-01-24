#include <Arduino.h>
#include <Arduino_APDS9960.h>

// Pick a Filter
//#define NXP_FILTER
#define MADGWICK  // My Choice, seems to work well and not a ton of CPU used
//#define MAHONY   

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

#ifdef NXP_FILTER
Adafruit_NXPSensorFusion nxpfilter;
#endif

bool blesenseboard = true;

int sense_Init()
{ 
    if (!IMU.begin()) {
        serialWriteln("Failed to initalize sensors");
        return -1;
    }  

    IMU.setGyroFS(2); // 1000dps gyro    
    IMU.setGyroODR(2); // 50hz

    // Increase Clock Speed, save some CPU due to blocking i2c
    Wire1.setClock(400000);  

#ifdef NXP_FILTER
    nxpfilter.begin(125); // Frequency discovered by oscilliscope
#else
#endif

    // Initalize Gesture Sensor
    if(!APDS.begin()) 
        blesenseboard = false;
    
    return 0;        
}


    float raccx=0,raccy=0,raccz=0;
    float rmagx=0,rmagy=0,rmagz=0;
    float rgyrx=0,rgyry=0,rgyrz=0;  
    float l_rgyrx=0,l_rgyry=0,l_rgyrz=0;  
    float accx=0,accy=0,accz=0;
    float magx=0,magy=0,magz=0;
    float gyrx=0,gyry=0,gyrz=0;
    float tilt=0,roll=0,pan=0;
    float rolloffset=0, panoffset=180, tiltoffset=0;
    float magxoff=0, magyoff=0, magzoff=0;
    float accxoff=0, accyoff=0, acczoff=0;
    float gyrxoff=0, gyryoff=0, gyrzoff=0;
    float rotatex=0,rotatety=0,rotatez=0;
    float l_panout=0, l_tiltout=0, l_rollout=0;

    PpmOut *ppmout;
    PpmIn *ppmin;
    uint16_t ppmchans[MAX_PPM_CHANNELS];
    SF fusion;
    Timer runt;
    
    int counter=0;
    int lastgesture = -1;


// Read all IMU data and do the calculations,
// This is run as a Thread at Real Time Priority
void sense_Thread()
{


//    while(1) {

        if(pauseThreads) {
            queue.call_in(std::chrono::milliseconds(100),sense_Thread);
            return;
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
                            accx, accy, accz, 
                            magx, magy, magz, 
        deltat);  
    #elif defined(MAHONY)
        fusion.MahonyUpdate(gyrx* DEG_TO_RAD, gyry* DEG_TO_RAD, gyrz* DEG_TO_RAD, accx, accy, accz, magx, magy, magz, deltat);  //else use the magwick, it is slower but more accurate
    #endif    

        roll = fusion.getPitch();
        tilt = fusion.getRoll();
        pan = fusion.getYaw();
    #endif

        // Reset Center on Wave
        if(blesenseboard) {
            bool btnpress=false;
            if (trkset.resetOnWave() && APDS.gestureAvailable()) {
                // a gesture was detected, read and print to serial monitor                
                int gesture = APDS.readGesture();               
                if(lastgesture == GESTURE_DOWN && 
                   gesture == GESTURE_UP)
                    btnpress=true;
                else if(lastgesture == GESTURE_UP && 
                        gesture == GESTURE_DOWN)
                    btnpress=true;
                else if(lastgesture == GESTURE_LEFT && 
                        gesture == GESTURE_RIGHT)
                    btnpress=true;
                else if(lastgesture == GESTURE_RIGHT && 
                        gesture == GESTURE_LEFT)
                    btnpress=true;
                lastgesture = gesture;
                if(btnpress) {
                    pressButton();
                    serialWriteln("HT: Reset center from a wave");
                }
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
        float beta = (float)trkset.lpTiltRoll() / 100;                        // LP Beta
        tiltout = beta * tiltout + (1.0 - beta) * l_tiltout;                  // Low Pass
        l_tiltout = tiltout;
        uint16_t tiltout_ui = tiltout + trkset.Tlt_cnt();                     // Apply Center Offset
        tiltout_ui = MAX(MIN(tiltout_ui,trkset.Tlt_max()),trkset.Tlt_min());  // Limit Output
               
        // Roll output
        float rollout = (roll - rolloffset) * trkset.Rll_gain() * (trkset.isRollReversed()? -1.0:1.0);
        rollout = beta * rollout + (1.0 - beta) * l_rollout;                  // Low Pass        
        l_rollout = rollout;
        uint16_t rollout_ui = rollout + trkset.Rll_cnt();                     // Apply Center Offset
        rollout_ui = MAX(MIN(rollout_ui,trkset.Rll_max()),trkset.Rll_min());  // Limit Output
        
        // Pan output, Normalize to +/- 180 Degrees
        float panout = normalize((pan-panoffset),-180,180)  * trkset.Pan_gain() * (trkset.isPanReversed()? -1.0:1.0);    
        beta = (float)trkset.lpPan() / 100;                                // LP Beta
        panout = beta * panout + (1.0 - beta) * l_panout;                  // Low Pass
        l_panout = panout;
        uint16_t panout_ui = panout + trkset.Pan_cnt();                    // Apply Center Offset
        panout_ui = MAX(MIN(panout_ui,trkset.Pan_max()),trkset.Pan_min()); // Limit Output

        // Reset all PPM Channels to Center
        for(int i=0;i<MAX_PPM_CHANNELS;i++)
            ppmchans[i] = TrackerSettings::DEF_CENTER;

        // Read all PPM inputs, if enabled
        ppmin = trkset.getPpmIn();
        if(ppmin != nullptr ) {    
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
        // THIS CODE SHOULD BE FASTER DOWN HERE, NoN-Blocking I2C eventually?

        if(++counter == SENSEUPDATE) { // Run Filter 5 more often than measurements
            counter = 0;
            // Accelerometer
            if(IMU.accelerationAvailable()) {
                IMU.readRawAccel(raccx, raccy, raccz);            
                raccx *= -1.0; // Flip X to make classic cartesian (+X Right, +Y Up, +Z Vert)
                trkset.accOffset(accxoff,accyoff,acczoff);
                accx = raccx - accxoff;
                accy = raccy - accyoff;
                accz = raccz - acczoff;
            }

            // Gyrometer
            if(IMU.gyroscopeAvailable()) {
                IMU.readRawGyro(rgyrx,rgyry,rgyrz);
                rgyrx *= -1.0; // Flip X to make classic cartesian (+X Right, +Y Up, +Z Vert)
                trkset.gyroOffset(gyrxoff,gyryoff,gyrzoff);            
                gyrx = rgyrx - gyrxoff; 
                gyry = rgyry - gyryoff; 
                gyrz = rgyrz - gyrzoff; 

                // Deadband on GyroZ
                if(fabs(gyrz) < GYRO_DEADBAND)
                    gyrz = 0;
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
            }
        }        

        // Update the settings
        // Both data and sensor threads will use this data. If data thread has it locked skip this reading.
        if(dataMutex.trylock()) {
            // Raw values for calibration
            trkset.setRawAccel(raccx,raccy,raccz);
            trkset.setRawGyro(rgyrx,rgyry,rgyrz);
            trkset.setRawMag(rmagx,rmagy,rmagz);

            // Offset values for debug
            //trkset.setOffAccel(accx,accy,accz);
            trkset.setOffGyro(gyrx,gyry,gyrz);
            trkset.setOffMag(magx,magy,magz);
            
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
        int sleepyt = (SENSE_PERIOD - micros); // Ideally 5ms period

        // Don't sleep for too short it something wrong above
        if(sleepyt < 5)
            sleepyt = 5;

        // **** Not super accurate being only in ms values
        // Not sure why but +1 works out on time

        queue.call_in(std::chrono::milliseconds(sleepyt/1000+1),sense_Thread);
        //ThisThread::sleep_for(std::chrono::milliseconds(sleepyt/1000+1)); 

    //} // END THREAD LOOP
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