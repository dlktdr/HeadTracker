/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2021 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
#include "SBUS/uarte_sbus.h"
#include "PWM/pmw.h"

#ifdef NXP_FILTER
Adafruit_NXPSensorFusion nxpfilter;
#endif

static float auxdata[10];
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

// Input Channel Data
static uint16_t ppm_in_chans[16];
static uint16_t sbus_in_chans[16];
static uint16_t bt_in_chans[BT_CHANNELS];

// Output channel data
static uint16_t channel_data[16];

Madgwick madgwick;
static Timer runt;
static Timer blefreq;

static int counter=0;
static bool blesenseboard=true;
static bool lastproximity=false;

void MagCalc();

// Initial Orientation Data+Vars
#define MADGINIT_ACCEL 0x01
#define MADGINIT_MAG   0x02
#define MADGSTART_SAMPLES 10
#define MADGINIT_READY (MADGINIT_ACCEL|MADGINIT_MAG)
static int madgreads=0;
static uint8_t madgsensbits=0;
static bool firstrun=true;
static float aacc[3]={0,0,0};
static float amag[3]={0,0,0};

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

    return 0;
}

// Read all IMU data and do the calculations,
void sense_Thread()
{
    if(pauseThreads) {
        queue.call_in(std::chrono::milliseconds(100),sense_Thread);
        return;
    }

    // Used to measure how long this took to adjust sleep timer
    runt.reset();
    runt.start();

    // Run this first to keep most accurate timing
#ifdef NXP_FILTER
    // NXP
    nxpfilter.update(gyrx, gyry, gyrz, accx, accy, accz, magx, magy, magz);
    roll = nxpfilter.getPitch();
    tilt = nxpfilter.getRoll();
    pan = nxpfilter.getYaw();
    if(madgreads == 10)
        nxpfilter.reset(); // 10 samples, then reset
    else if(madgreads < 1500)
        panoffset = pan; // Latch output at zero
    else if(madgreads > 1500) // Prevent loop
        madgreads = 1500;
    madgreads++;

#else
    // Period Between Samples
    float deltat = madgwick.deltatUpdate();

    // Only do this update after the first mag and accel data have been read.
    if(madgreads == 0) {
        if(madgsensbits == MADGINIT_READY) {
            madgsensbits = 0;
            madgreads++;
            aacc[0] = accx; aacc[1] = accy;  aacc[2] = accz;
            amag[0] = magx; amag[1] = magy;  amag[2] = magz;

        }
    } else if(madgreads < MADGSTART_SAMPLES-1) {
        if(madgsensbits == MADGINIT_READY) {
            madgsensbits = 0;
            madgreads++;
            aacc[0] += accx; aacc[1] += accy;  aacc[2] += accz;
            aacc[0] /= 2;    aacc[1] /= 2;     aacc[2] /= 2;
            amag[0] += magx; amag[1] += magy;  amag[2] += magz;
            amag[0] /= 2;    amag[1] /= 2;     amag[2] /= 2;
        }

    // Got 10 Values
    } else if(madgreads == MADGSTART_SAMPLES-1) {
        // Pass it averaged values
        madgwick.begin(aacc[0], aacc[1], aacc[2], amag[0], amag[1], amag[2]);
        panoffset = pan;
        madgreads = MADGSTART_SAMPLES;
    }

    // Apply initial orientation
    if(madgreads == MADGSTART_SAMPLES) {
        madgwick.update(gyrx * DEG_TO_RAD, gyry * DEG_TO_RAD, gyrz * DEG_TO_RAD,
                        accx, accy, accz,
                        magx, magy, magz,
                        deltat);
        roll = madgwick.getPitch();
        tilt = madgwick.getRoll();
        pan = madgwick.getYaw();

        if(firstrun) {
            panoffset = pan;
            firstrun = false;
        }
    }
#endif

    // Reset Center on Proximity, Don't need to update this often
    static int sensecount=0;
    static int minproximity=100; // Keeps smallest proximity read.
    static int maxproximity=0; // Keeps largest proximity value read.
    if(blesenseboard && sensecount++ >= 50) {
        sensecount = 0;
        if (trkset.resetOnWave()) {
            // Reset on Proximity
            if(APDS.proximityAvailable()) {
                int proximity = APDS.readProximity();

                // Store High and Low Values, Generate reset thresholds
                maxproximity = MAX(proximity, maxproximity);
                minproximity = MIN(proximity, minproximity);
                int lowthreshold = minproximity + APDS_HYSTERISIS;
                int highthreshold = maxproximity - APDS_HYSTERISIS;

                // Don't allow reset if high and low thresholds are too close
                if(highthreshold - lowthreshold > APDS_HYSTERISIS*2) {
                    if (proximity < lowthreshold && lastproximity == false) {
                        pressButton();
                        serialWriteln("HT: Reset center from a close proximity");
                        lastproximity = true;
                    } else if(proximity > highthreshold) {
                        // Clear flag on proximity clear
                        lastproximity = false;
                    }
                }
            }
        }
    }

    // Get the current Bluetooth Function
    BTFunction *btf = trkset.getBTFunc();

    // Zero button was pressed, adjust all values to zero
    bool butdnw = false;
    if(wasButtonPressed()) {
        rolloffset = roll;
        panoffset = pan;
        tiltoffset = tilt;
        butdnw = true;
    }

    // If button was pressed and this is a remote bluetooth send the button press back
    static bool btbtnupdated=false;
    if(btf != nullptr) {
        if(btf->mode() == BTPARARMT) {
            BTParaRmt *rmt = static_cast<BTParaRmt*>(btf);
            if(butdnw && btbtnupdated == false) {
                rmt->sendButtonData('R');
                btbtnupdated = true;
            }
            else if(btbtnupdated == true) {
                rmt->sendButtonData(0);
                btbtnupdated = false;
            }
        }
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


    /* ************************************************************
     *       Build channel data
     *
     * Build Channel Data
     *   1) Reset all channels to center
     *   2) Set PPMin channels if available
     *   3) Set SBUSin channels if available
     *   4) Set received BT channels if available
     *   5) Reset Center on PPM channel if enabled
     *   6) Set auxiliary functions
     *   7) Set analog channels
     *   8) Override desired channels with pan/tilt/roll
     *   9) Output to PPMout
     *  10) Output to Bluetooth
     *  11) Output to SBUS
     *  12) Output PWM channels
     *
     *  Channels should all be set to zero if they don't have valid data
     *  Only on the output should a channel be set to center if it's still zero
     *  Allows the GUI to know which channels are valid
     */

    // 1) Reset all Channels to zero which means they have no data
    for(int i=0;i<16;i++)
        channel_data[i] = 0;

    // 2) Read all PPM inputs
    PpmIn_execute();
    for(int i=0;i<16;i++)
        ppm_in_chans[i] = 0;    // Reset all PPM in channels to Zero (Not active)
    int ppm_in_chcnt = PpmIn_getChannels(ppm_in_chans);
    if(ppm_in_chcnt >= 4 && ppm_in_chcnt <= 16) {
        for(int i=0;i<MIN(ppm_in_chcnt,16);i++) {
            channel_data[i] = ppm_in_chans[i];
        }
    }

    // 3) Set all incoming SBUS values

    //*** Todo

    // 4) Set all incoming BT values
    // Bluetooth cannot send a zero value for a channel. Radios see this as invalid data.
    // So, if the data is coming from a BLE head unit it also has a characteristic to nofity which ones are valid
    // allowing PPM/SBUS pass through on the head or remote boards
    // If the data is coming from a PARA radio all 8ch's are going to have values, all PPM/SBUS inputs 1-8 will be overridden

    for(int i=0;i<BT_CHANNELS;i++)
        bt_in_chans[i] = 0;    // Reset all BT in channels to Zero (Not active)
    if(btf != nullptr) {
        for(int i=0;i<BT_CHANNELS;i++) {
            bt_in_chans[i] = btf->getChannel(i); // Read the BT Data
            if(bt_in_chans[i] > 0) {
                channel_data[i] = bt_in_chans[i];
            }
        }
    }

    // 5) If selected input channel went > 1800us reset the center
    // wait for it to drop below 1700 before allowing another reset
    int rstppmch = trkset.resetCntPPM() - 1;
    static bool hasrstppm=false;
    if(rstppmch >= 0 && rstppmch < 16) {
        if(channel_data[rstppmch] > 1800 && hasrstppm == false) {
            serialWrite("HT: Reset Center - Input Channel ");
            serialWrite(rstppmch+1);
            serialWriteln(" > 1800us");
            pressButton();
            hasrstppm = true;
        } else if (channel_data[rstppmch] < 1700 && hasrstppm == true) {
            hasrstppm = false;
        }
    }

    // 6) Set Auxiliary Functions
    int aux0ch = trkset.auxFunc0Ch();
    int aux1ch = trkset.auxFunc1Ch();
    if(aux0ch > 0 || aux1ch > 0) {
        buildAuxData();
        if(aux0ch > 0)
            channel_data[aux0ch - 1] = auxdata[trkset.auxFunc0()];
        if(aux1ch > 0)
            channel_data[aux1ch - 1] = auxdata[trkset.auxFunc1()];
    }

    // 7) Set Analog Channels
    if(trkset.analog6Ch() > 0) {
        float an6 = analogRead(6);
        an6 /= 1241.2f; // Equals 0-3.3V w/High resolution Analog Read
        an6 *= trkset.analog6Gain();
        an6 += trkset.analog6Offset();
        an6 += TrackerSettings::MIN_PWM;
        an6 = MAX(TrackerSettings::MIN_PWM,MIN(TrackerSettings::MAX_PWM,an6));
        channel_data[trkset.analog6Ch()-1] = an6;
    }
    if(trkset.analog7Ch() > 0) {
        float an7 = analogRead(7);
        an7 /= 1241.2f; // Equals 0-3.3V w/High resolution Analog Read
        an7 *= trkset.analog7Gain();
        an7 += trkset.analog7Offset();
        an7 += TrackerSettings::MIN_PWM;
        an7 = MAX(TrackerSettings::MIN_PWM,MIN(TrackerSettings::MAX_PWM,an7));
        channel_data[trkset.analog7Ch()-1] = an7;
    }

    // 8) Set PPM Channel Values if enabled
    int tltch = trkset.tiltCh();
    int rllch = trkset.rollCh();
    int panch = trkset.panCh();
    if(tltch > 0)
        channel_data[tltch - 1] = tiltout_ui; // Channel 1 = Index 0
    if(rllch > 0)
        channel_data[rllch - 1] = rollout_ui;
    if(panch > 0)
        channel_data[panch - 1] = panout_ui;

    // 9) Set the PPM Outputs
    for(int i=0;i<PpmOut_getChnCount();i++) {
        uint16_t ppmout = channel_data[i];
        if(ppmout == 0)
            ppmout = TrackerSettings::PPM_CENTER;
        PpmOut_setChannel(i,ppmout);
    }

    // 10) Set all the BT Channels, send the zeros.
    bool bleconnected=false;
    BTFunction *bt = trkset.getBTFunc();
    if(bt != nullptr) {
        bleconnected = bt->isConnected();
        trkset.setBLEAddress(bt->address());
        for(int i=0;i < BT_CHANNELS;i++) {
            bt->setChannel(i,channel_data[i]);
        }
    }

    // 11) Set all SBUS output channels, if disabled set to center
    uint16_t sbus_data[16];
    for(int i=0;i<16;i++) {
        uint16_t sbusout = channel_data[i];
        if(sbusout == 0)
            sbusout = TrackerSettings::PPM_CENTER;
        sbus_data[i] = (static_cast<float>(sbusout) - TrackerSettings::PPM_CENTER) * TrackerSettings::SBUS_SCALE + TrackerSettings::SBUS_CENTER;
    }
    SBUS_TX_BuildData(sbus_data);

    // 12) Set PWM Channels
    for(int i=0;i<4;i++) {
        int pwmch = trkset.PWMCh(i)-1;
        if(pwmch >= 0 && pwmch < 16) {
            uint16_t pwmout = channel_data[pwmch];
            if(pwmout == 0)
                pwmout = TrackerSettings::PPM_CENTER;
            setPWMValue(i,pwmout);
        }
    }

    /* ************************************************************
     * Sensor Reading Below, done after channels for timing reasons
     *    as below code can vary wildly. So delayed by one sample
     ************************************************************/

    // If not using any of the tilt roll or pan outputs
    // Don't bother wasting time calculating the tilt/roll/pan outputs
    // If ui connected, do it so you can still see the outputs.

    if(tltch < 0 && panch < 0 && rllch < 0 && uiconnected == false)
        counter = -1;

    // Oversampling.
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

            // For intial orientation setup
            madgsensbits |= MADGINIT_ACCEL;
        }

        // Gyrometer
        if(IMU.gyroscopeAvailable()) {
            IMU.readRawGyro(rgyrx,rgyry,rgyrz);
            rgyrx *= -1.0; // Flip X to match other sensors
            trkset.gyroOffset(gyrxoff,gyryoff,gyrzoff);
            gyrx = rgyrx - gyrxoff;
            gyry = rgyry - gyryoff;
            gyrz = rgyrz - gyrzoff;

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

            // For inital orientation setup
            madgsensbits |= MADGINIT_MAG;
        }
    }

    // Update the settings for the GUI
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
        trkset.setPPMInValues(ppm_in_chans);
        trkset.setBLEValues(bt_in_chans);
        trkset.setSBUSValues(sbus_in_chans);
        trkset.setChannelOutValues(channel_data);

        // Qauterion Data
        //float *qd = madgwick.getQuat();
        //trkset.setQuaternion(qd);

        // Bluetooth connected
        trkset.setBlueToothConnected(bleconnected);

        dataMutex.unlock();
    }


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
    if(rotation[0] != 0) {
        out[0] = pn[0] * 1 + pn[1] * 0            + pn[2] * 0;
        out[1] = pn[0] * 0 + pn[1] * cos(rot[0])  - pn[2] * sin(rot[0]);
        out[2] = pn[0] * 0 + pn[1] * sin(rot[0])  + pn[2] * cos(rot[0]);
        memcpy(pn,out,sizeof(out[0])*3);
    }

    // Y Rotation
    if(rotation[1] != 0) {
        out[0] =  pn[0] * cos(rot[1]) - pn[1] * 0 + pn[2] * sin(rot[1]);
        out[1] =  pn[0] * 0           + pn[1] * 1 + pn[2] * 0;
        out[2] = -pn[0] * sin(rot[1]) + pn[1] * 0 + pn[2] * cos(rot[1]);
        memcpy(pn,out,sizeof(out[0])*3);
    }

    // Z Rotation
    if(rotation[2] != 0) {
        out[0] = pn[0] * cos(rot[2]) - pn[1] * sin(rot[2]) + pn[2] * 0;
        out[1] = pn[0] * sin(rot[2]) + pn[1] * cos(rot[2]) + pn[2] * 0;
        out[2] = pn[0] * 0           + pn[1] * 0           + pn[2] * 1;
        memcpy(pn,out,sizeof(out[0])*3);
    }
}

/* reset_fusion()
 *      Causes the madgwick filter to reset. Used when board rotation changes
 */

void reset_fusion()
{
    madgreads = 0;
    madgsensbits = 0;
    firstrun = true;
    aacc[0] = 0; aacc[1] = 0; aacc[2] = 0;
    amag[0] = 0; amag[1] = 0; amag[2] = 0;
}

/* Builds data for auxiliary functions
*/

void buildAuxData()
{
    float pwmrange = (TrackerSettings::MAX_PWM - TrackerSettings::MIN_PWM);
    auxdata[0] = (gyrx / 1000) * pwmrange + TrackerSettings::PPM_CENTER;
    auxdata[1] = (gyry / 1000) * pwmrange + TrackerSettings::PPM_CENTER;
    auxdata[2] = (gyrz / 1000) * pwmrange + TrackerSettings::PPM_CENTER;
    auxdata[3] = (accx / 2.0f) * pwmrange + TrackerSettings::PPM_CENTER;
    auxdata[4] = (accy / 2.0f) * pwmrange + TrackerSettings::PPM_CENTER;
    auxdata[5] = (accz / 1.0f) * pwmrange + TrackerSettings::PPM_CENTER;
    auxdata[6] = ((accz -1.0f) / 2.0f) * pwmrange + TrackerSettings::PPM_CENTER;
    auxdata[7] = static_cast<float>(BLE.central().rssi()) / 127.0 * pwmrange + TrackerSettings::MIN_PWM;
}