#include <Arduino.h>
#include <Arduino_APDS9960.h>

// Pick a Filter
//#define NXP_FILTER
#define MADGWICK  // My Choice, seems to work well and not a ton of CPU used

#include "trackersettings.h"
#include "sense.h"
#include "mbed.h"
#include "main.h"
#include "dataparser.h"
#include "LSM9DS1/Arduino_LSM9DS1.h"
#include "serial.h"
#include "Wire.h"
#include "ble.h"
#include "NXPFusion/Adafruit_AHRS_NXPFusion.h"
#include "MadgwickAHRS/MadgwickAHRS.h"

#ifdef NXP_FILTER
Adafruit_NXPSensorFusion nxpfilter;
#endif

static float raccx=0,raccy=0,raccz=0;
static float rmagx=0,rmagy=0,rmagz=0;
static float rgyrx=0,rgyry=0,rgyrz=0;
static float accx=0,accy=0,accz=0;
static float magx=0,magy=0,magz=0;
static float gyrx=0,gyry=0,gyrz=0;
static float tilt=0,roll=0,pan=0;
static float rolloffset=0, panoffset=0, tiltoffset=0;
static float magxoff=0, magyoff=0, magzoff=0;
static float accxoff=0, accyoff=0, acczoff=0;
static float gyrxoff=0, gyryoff=0, gyrzoff=0;
static float l_panout=0, l_tiltout=0, l_rollout=0;

uint16_t zaccelout=1500;

static uint16_t ppmchans[MAX_PPM_CHANNELS];
Madgwick madgwick;
static Timer runt;

static int counter=0;
static bool blesenseboard=true;
static bool lastproximity=false;

void MagCalc();

int sense_Init()
{
    if (!IMU.begin()) {
        serialWriteln("Failed to initalize sensors");
        return -1;
    }

    // Increase Clock Speed, save some CPU due to blocking i2c
    Wire1.setClock(400000);

#ifdef NXP_FILTER
    nxpfilter.begin(125); // Frequency discovered by oscilliscope
#else
#endif

    // Initalize Gesture Sensor
    if(!APDS.begin())
        blesenseboard = false;


    // Initalize Quaternion to The Startup angle
    while(!IMU.accelAvailable());
    IMU.readRawAccel(raccx, raccy, raccz);
    while(!IMU.magnetAvailable());
    IMU.readRawMagnet(rmagx, rmagy, rmagz);
    trkset.magOffset(magxoff,magyoff,magzoff);
    // Calibrate Hard Iron Offsets
    magx = rmagx - magxoff;
    magy = rmagy - magyoff;
    magz = rmagz - magzoff;
    madgwick.begin(-raccx, raccy, raccz, rmagx, rmagy, rmagz);

    return 0;
}

// Read all IMU data and do the calculations,
// This is run as a Thread at Real Time Priority
void sense_Thread()
{
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
    float deltat = madgwick.deltatUpdate();
#ifdef MADGWICK
    madgwick.update(gyrx * DEG_TO_RAD, gyry * DEG_TO_RAD, gyrz * DEG_TO_RAD,
                        accx, accy, accz,
                        magx, magy, magz,
                        deltat);
#elif defined(MAHONY)
    fusion.MahonyUpdate(gyrx* DEG_TO_RAD, gyry* DEG_TO_RAD, gyrz* DEG_TO_RAD, accx, accy, accz, magx, magy, magz, deltat);  //else use the magwick, it is slower but more accurate
#endif
    roll = madgwick.getPitch();
    tilt = madgwick.getRoll();
    pan = madgwick.getYaw();
    static bool firstrun=true;
    if(firstrun) {
        panoffset = pan;
        firstrun = false;
    }
#endif

    // Reset Center on Wave or Proximity, Don't need to updated this often
    static int sensecount=0;
    if(blesenseboard && sensecount++ == 50) {
        sensecount = 0;
        //bool btnpress=false;
        if (trkset.resetOnWave()) {
            // Reset on Proximity
            if(APDS.proximityAvailable()) {
                int proximity = APDS.readProximity();

                if (proximity == 0 && lastproximity == false) {
                    pressButton();
                    serialWriteln("HT: Reset center from a close proximity");
                    lastproximity = true;
                } else if(proximity > 200) {
                    // Clear flag on proximity clear
                    lastproximity = false;
                }
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

    // Read all PPM inputs, should return 0 channels if disabled or lost
    PpmIn_execute();
    uint16_t ppminchans[16];
    int ppmi_chcnt = PpmIn_getChannels(ppminchans);
    if(ppmi_chcnt >= 4 && ppmi_chcnt <= 16) {
        for(int i=0;i<MIN(ppmi_chcnt,MAX_PPM_CHANNELS);i++) {
            ppmchans[i] = ppminchans[i];
        }
    } else {
        ppmi_chcnt = 0; // Not within range set to zero
    }

    // If selected PPM input channel went > 1800us reset the center
    // wait for it to drop below 1700 before allowing another reset
    int rstppmch = trkset.resetCntPPM() - 1;
    static bool hasrstppm=false;
    if(rstppmch >= 0 && rstppmch < MAX_PPM_CHANNELS) {
        if(ppmchans[rstppmch] > 1800 && hasrstppm == false) {
            serialWrite("HT: Reset Center - PPM Channel ");
            serialWrite(rstppmch+1);
            serialWriteln(" > 1800us");
            pressButton();
            hasrstppm = true;
        } else if (ppmchans[rstppmch] < 1700 && hasrstppm == true) {
            hasrstppm = false;
        }
    }

    // Set PPM Channel Values
    ppmchans[trkset.tiltCh()-1] = tiltout_ui; // Channel 1 = Index 0
    ppmchans[trkset.rollCh()-1] = rollout_ui;
    ppmchans[trkset.panCh()-1] = panout_ui;

    // Z Acceleration output on channel 4
    //ppmchans[3] = MIN(MAX((((raccz  - 1.0) / 2) * 1000)+1500,TrackerSettings::MIN_PWM),TrackerSettings::MAX_PWM);
    //zaccelout = ppmchans[3];

    // Set the PPM Outputs
    for(int i=0;i<PpmOut_getChnCount();i++) {
        PpmOut_setChannel(i,ppmchans[i]);
    }

    // Set all the BT Channels
    BTFunction *bt = trkset.getBTFunc();
    if(bt != nullptr) {
        trkset.setBLEAddress(bt->address());
        for(int i=0;i<MAX_PPM_CHANNELS;i++) {
            bt->setChannel(i,ppmchans[i]);
        }
    }

    digitalWrite(A0,LOW); // Pin for timing check
    digitalWrite(A1,HIGH);

    // Get new data from sensors..
    //  FIX MEE I2C is hogging 40% of processor just waiting!!! UGG Needs to be changed to non-blocking

    if(++counter == SENSEUPDATE) {
        counter = 0;

        // Setup Rotations
        float rotation[3];
        trkset.orientRotations(rotation);

        // Accelerometer
        if(IMU.accelerationAvailable()) {
            IMU.readRawAccel(raccx, raccy, raccz);
            raccx *= -1.0; // Flip X to make classic cartesian (+X Right, +Y Up, +Z Vert)
            trkset.accOffset(accxoff,accyoff,acczoff);

            accx = raccx - accxoff;
            accy = raccy - accyoff;
            accz = raccz - acczoff;

            // Apply Rotation
            float tmpacc[3] = {accx,accy,accz};
            rotate(tmpacc,rotation);
            accx = tmpacc[0]; accy = tmpacc[1]; accz = tmpacc[2];
        }

        // Gyrometer
        if(IMU.gyroscopeAvailable()) {
            IMU.readRawGyro(rgyrx,rgyry,rgyrz);
            rgyrx *= -1.0; // Flip X to match other sensors
            trkset.gyroOffset(gyrxoff,gyryoff,gyrzoff);
            gyrx = rgyrx - gyrxoff;
            gyry = rgyry - gyryoff;
            gyrz = rgyrz - gyrzoff;

            // Deadband on GyroZ
            if(fabs(gyrz) < GYRO_DEADBAND)
                gyrz = 0;

            // Apply Rotation
            float tmpgyr[3] = {gyrx,gyry,gyrz};
            rotate(tmpgyr,rotation);
            gyrx = tmpgyr[0]; gyry = tmpgyr[1]; gyrz = tmpgyr[2];
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
            magx = (magx * magsioff[0]) + (magy * magsioff[1]) + (magz * magsioff[2]);
            magy = (magx * magsioff[3]) + (magy * magsioff[4]) + (magz * magsioff[5]);
            magz = (magx * magsioff[6]) + (magy * magsioff[7]) + (magz * magsioff[8]);

            // Apply Rotation
            float tmpmag[3] = {magx,magy,magz};
            rotate(tmpmag, rotation);
            magx = tmpmag[0]; magy = tmpmag[1]; magz = tmpmag[2];
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
        trkset.setOffAccel(accx,accy,accz);
        trkset.setOffGyro(gyrx,gyry,gyrz);
        trkset.setOffMag(magx,magy,magz);

        trkset.setRawOrient(tilt,roll,pan);
        trkset.setOffOrient(tilt-tiltoffset,roll-rolloffset, normalize(pan-panoffset,-180,180));
        trkset.setPPMOut(tiltout_ui,rollout_ui,panout_ui);

        // PPM Input Values
        trkset.setPPMInValues(ppminchans,ppmi_chcnt);

        // Qauterion Data
        float *qd = madgwick.getQuat();
        trkset.setQuaternion(qd);

        dataMutex.unlock();
    }

    digitalWrite(A1,HIGH);


    // Use this time to determine how long to sleep for, save in microseconds in sleepyt
    runt.stop();
    using namespace std::chrono;
    int micros = duration_cast<microseconds>(runt.elapsed_time()).count();
    int sleepyt = (SENSE_PERIOD - micros);

    // Don't sleep for too short it something wrong above
    if(sleepyt < 5)
        sleepyt = 5;

    // **** Not super accurate being only in ms values
    // Not sure why but +1 works out on time
    queue.call_in(std::chrono::milliseconds(sleepyt/1000+1),sense_Thread);
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

// Rotate, in Order X -> Y -> Z

void rotate(float pn[3], const float rotation[3])
{
    float rot[3] = {0,0,0};
    memcpy(rot,rotation,sizeof(rot[0])*3);

    // Passed in Degrees
    rot[0] *= DEG_TO_RAD;
    rot[1] *= DEG_TO_RAD;
    rot[2] *= DEG_TO_RAD;

    float out[3];

    // X Rotation
    out[0] = pn[0]*1 + pn[1]*0            + pn[2]*0;
    out[1] = pn[0]*0 + pn[1]*cos(rot[0])  - pn[2]*sin(rot[0]);
    out[2] = pn[0]*0 + pn[1]*sin(rot[0])  + pn[2]*cos(rot[0]);
    memcpy(pn,out,sizeof(out[0])*3);

    // Y Rotation
    out[0] =  pn[0]*cos(rot[1]) - pn[1]*0 + pn[2]*sin(rot[1]);
    out[1] =  pn[0]*0           + pn[1]*1 + pn[2]*0;
    out[2] = -pn[0]*sin(rot[1]) + pn[1]*0 + pn[2]*cos(rot[1]);
    memcpy(pn,out,sizeof(out[0])*3);

    // Z Rotation
    out[0] = pn[0]*cos(rot[2]) - pn[1]*sin(rot[2]) + pn[2]*0;
    out[1] = pn[0]*sin(rot[2]) + pn[1]*cos(rot[2]) + pn[2]*0;
    out[2] = pn[0]*0           + pn[1]*0           + pn[2]*1;
    memcpy(pn,out,sizeof(out[0])*3);
}