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

#include "sense.h"

#include <device.h>
#include <drivers/sensor.h>
#include <zephyr.h>

#include "APDS9960/APDS9960.h"
#include "LSM9DS1/LSM9DS1.h"
#include "MadgwickAHRS/MadgwickAHRS.h"
#include "SBUS/sbus.h"
#include "analog.h"
#include "ble.h"
#include "filters.h"
#include "filters/SF1eFilter.h"
#include "io.h"
#include "joystick.h"
#include "log.h"
#include "nano33ble.h"
#include "pmw.h"
#include "trackersettings.h"

static float auxdata[10];
static float raccx = 0, raccy = 0, raccz = 0;
static float rmagx = 0, rmagy = 0, rmagz = 0;
static float rgyrx = 0, rgyry = 0, rgyrz = 0;
static float accx = 0, accy = 0, accz = 0;
static float magx = 0, magy = 0, magz = 0;
static float gyrx = 0, gyry = 0, gyrz = 0;
static float tilt = 0, roll = 0, pan = 0;
static float rolloffset = 0, panoffset = 0, tiltoffset = 0;
static float magxoff = 0, magyoff = 0, magzoff = 0;
static float accxoff = 0, accyoff = 0, acczoff = 0;
static float gyrxoff = 0, gyryoff = 0, gyrzoff = 0;
static float l_panout = 0, l_tiltout = 0, l_rollout = 0;
static bool trpOutputEnabled = false;  // Default to disabled T/R/P output
volatile bool gyro_calibrated = false;

// Input Channel Data
static uint16_t ppm_in_chans[16];
static uint16_t sbus_in_chans[16];
static uint16_t bt_chans[BT_CHANNELS];
static float bt_chansf[BT_CHANNELS];

// Output channel data
static uint16_t channel_data[16];

Madgwick madgwick;

int64_t usduration = 0;

static bool blesenseboard = false;
static bool lastproximity = false;

K_MUTEX_DEFINE(sensor_mutex);

LSM9DS1Class IMU;

// Initial Orientation Data+Vars
#define MADGINIT_ACCEL 0x01
#define MADGINIT_MAG 0x02

#define MADGINIT_READY (MADGINIT_ACCEL | MADGINIT_MAG)
static int madgreads = 0;
static uint8_t madgsensbits = 0;
static volatile bool firstrun = true;
static float aacc[3] = {0, 0, 0};
static float amag[3] = {0, 0, 0};

// Analog Filters
SF1eFilter *anFilter[AN_CH_CNT];

volatile bool senseTreadRun = false;

int sense_Init()
{
  if (!IMU.begin()) {
    LOGE("Failed to initalize sensors");
    return -1;
  }

  // Initalize Gesture Sensor
  if (!APDS.begin()) {
    blesenseboard = false;
    trkset.setSenseboard(false);
  } else {
    blesenseboard = true;
    trkset.setSenseboard(true);
  }

  for (int i = 0; i < BT_CHANNELS; i++) {
    bt_chansf[i] = 0;
  }

  // Create analog filters
  for (int i = 0; i < AN_CH_CNT; i++) {
    anFilter[i] = SF1eFilterCreate(AN_FILT_FREQ, AN_FILT_MINCO, AN_FILT_SLOPE, AN_FILT_DERCO);
    SF1eFilterInit(anFilter[i]);
  }

  setLEDFlag(LED_GYROCAL);

  gyro_calibrated = false;
  senseTreadRun = true;

  return 0;
}

//----------------------------------------------------------------------
// Calculations and Main Channel Thread
//----------------------------------------------------------------------

void calculate_Thread()
{
  while (1) {
    if (!senseTreadRun) {
      rt_sleep_ms(10);
      continue;
    }

    usduration = micros64();

    // Period Between Samples
    float deltat = madgwick.deltatUpdate();

    // Use a mutex so sensor data can't be updated part way
    k_mutex_lock(&sensor_mutex, K_FOREVER);

    // Only do this update after the first mag and accel data have been read.
    if (madgreads == 0) {
      if (madgsensbits == MADGINIT_READY) {
        madgsensbits = 0;
        madgreads++;
        aacc[0] = accx;
        aacc[1] = accy;
        aacc[2] = accz;
        amag[0] = magx;
        amag[1] = magy;
        amag[2] = magz;
      }

      // Average samples
    } else if (madgreads < MADGSTART_SAMPLES - 1) {
      if (madgsensbits == MADGINIT_READY) {
        madgsensbits = 0;
        madgreads++;
        aacc[0] += accx;
        aacc[1] += accy;
        aacc[2] += accz;
        aacc[0] /= 2;
        aacc[1] /= 2;
        aacc[2] /= 2;
        amag[0] += magx;
        amag[1] += magy;
        amag[2] += magz;
        amag[0] /= 2;
        amag[1] /= 2;
        amag[2] /= 2;
      }

      // Got the averaged values, apply the initial orientation.
    } else if (madgreads == MADGSTART_SAMPLES - 1) {
      // Pass it averaged values
      madgwick.begin(aacc[0], aacc[1], aacc[2], amag[0], amag[1], amag[2]);
      panoffset = pan;
      madgreads = MADGSTART_SAMPLES;
    }

    // Do the AHRS calculations
    if (madgreads == MADGSTART_SAMPLES) {
      madgwick.update(gyrx * DEG_TO_RAD, gyry * DEG_TO_RAD, gyrz * DEG_TO_RAD, accx, accy, accz,
                      magx, magy, magz, deltat);
      roll = madgwick.getPitch();
      tilt = madgwick.getRoll();
      pan = madgwick.getYaw();

      if (firstrun) {
        panoffset = pan;
        firstrun = false;
      }
    }

    // Free Mutex Lock, Allow sensor updates
    k_mutex_unlock(&sensor_mutex);

    // Re-apply inital orientation as soon as the gyro calibration is done
    // As is is a good time to be known sitting still.
    static bool lastgyrcal = false;
    if (gyro_calibrated == true && lastgyrcal == false) reset_fusion();
    lastgyrcal = gyro_calibrated;

    // Toggles output on and off if long pressed
    bool butlngdwn = false;
    if (wasButtonLongPressed()) {
      trpOutputEnabled = !trpOutputEnabled;
      butlngdwn = true;
    }

    static bool btbtnlngupdated = false;
    if (BTGetMode() == BTPARARMT) {
      if (butlngdwn && btbtnlngupdated == false) {
        BTRmtSendButtonPress(true);  // Send the long press over bluetooth to remote board
        btbtnlngupdated = true;
      } else if (btbtnlngupdated == true) {
        btbtnlngupdated = false;
      }
    }

    // Zero button was pressed, adjust all values to zero
    bool butdnw = false;
    if (wasButtonPressed()) {
      rolloffset = roll;
      panoffset = pan;
      tiltoffset = tilt;
      butdnw = true;
    }

    // If button was pressed and this is a remote bluetooth boart send the button press back
    static bool btbtnupdated = false;
    if (BTGetMode() == BTPARARMT) {
      if (butdnw && btbtnupdated == false) {
        BTRmtSendButtonPress(false);  // Send the short press over bluetooth to remote board
        btbtnupdated = true;
      } else if (btbtnupdated == true) {
        btbtnupdated = false;
      }
    }

    // Tilt output
    float tiltout =
        (tilt - tiltoffset) * trkset.Tlt_gain() * (trkset.isTiltReversed() ? -1.0 : 1.0);
    float beta = (float)trkset.lpTiltRoll() / 100;  // LP Beta
    filter_expAverage(&tiltout, beta, &l_tiltout);
    uint16_t tiltout_ui = tiltout + trkset.Tlt_cnt();                       // Apply Center Offset
    tiltout_ui = MAX(MIN(tiltout_ui, trkset.Tlt_max()), trkset.Tlt_min());  // Limit Output

    // Roll output
    float rollout =
        (roll - rolloffset) * trkset.Rll_gain() * (trkset.isRollReversed() ? -1.0 : 1.0);
    filter_expAverage(&rollout, beta, &l_rollout);
    uint16_t rollout_ui = rollout + trkset.Rll_cnt();                       // Apply Center Offset
    rollout_ui = MAX(MIN(rollout_ui, trkset.Rll_max()), trkset.Rll_min());  // Limit Output

    // Pan output, Normalize to +/- 180 Degrees
    float panout = normalize((pan - panoffset), -180, 180) * trkset.Pan_gain() *
                   (trkset.isPanReversed() ? -1.0 : 1.0);
    filter_expAverage(&panout, (float)trkset.lpPan() / 100, &l_panout);
    uint16_t panout_ui = panout + trkset.Pan_cnt();                       // Apply Center Offset
    panout_ui = MAX(MIN(panout_ui, trkset.Pan_max()), trkset.Pan_min());  // Limit Output

    // Reset on tilt
    static bool doresetontilt = false;
    if (trkset.resetOnTiltMode()) {
      static bool tiltpeak = false;
      static float resettime = 0.0f;
      enum {
        HITNONE,
        HITMIN,
        HITMAX,
      };
      static int minmax = HITNONE;
      if (rollout_ui == trkset.Rll_max()) {
        if (tiltpeak == false && minmax == HITNONE) {
          tiltpeak = true;
          minmax = HITMAX;
        } else if (minmax == HITMIN) {
          minmax = HITNONE;
          tiltpeak = false;
          doresetontilt = true;
        }

      } else if (rollout_ui == trkset.Rll_min()) {
        if (tiltpeak == false && minmax == HITNONE) {
          tiltpeak = true;
          minmax = HITMIN;
        } else if (minmax == HITMAX) {
          minmax = HITNONE;
          tiltpeak = false;
          doresetontilt = true;
        }
      }

      // If hit a max/min wait an amount of time and reset it
      if (tiltpeak == true) {
        resettime += (float)CALCULATE_PERIOD / 1000000.0;
        if (resettime > TrackerSettings::RESET_ON_TILT_TIME) {
          tiltpeak = false;
          minmax = HITNONE;
          resettime = 0;
        }
      }
    }

    // Do the actual reset after a delay
    static float timetoreset = 0;
    if (doresetontilt) {
      if (timetoreset > TrackerSettings::RESET_ON_TILT_AFTER) {
        doresetontilt = false;
        timetoreset = 0;
        pressButton();
      }
      timetoreset += (float)CALCULATE_PERIOD / 1000000.0;
    }

    /* ************************************************************
     *       Build channel data
     *
     * Build Channel Data
     *   1) Reset all channels to disabled
     *   2) Set PPMin channels
     *   3) Set SBUSin channels
     *   4) Set received BT channels
     *   5) Reset Center on PPM channel
     *   6) Set auxiliary functions
     *   7) Set analog channels
     *   8) Set Reset Center pulse channel
     *   9) Override desired channels with pan/tilt/roll
     *  10) Output to PPMout
     *  11) Output to Bluetooth
     *  12) Output to SBUS
     *  13) Output PWM channels
     *  14) Output to USB Joystick
     *
     *  Channels should all be set to zero if they don't have valid data
     *  Only on the output should a channel be set to center if it's still zero
     *  Allows the GUI to know which channels are valid
     */

    // 1) Reset all Channels to zero which means they have no data
    for (int i = 0; i < 16; i++) channel_data[i] = 0;

    // 2) Read all PPM inputs
    PpmIn_execute();
    for (int i = 0; i < 16; i++)
      ppm_in_chans[i] = 0;  // Reset all PPM in channels to Zero (Not active)
    int ppm_in_chcnt = PpmIn_getChannels(ppm_in_chans);
    if (ppm_in_chcnt >= 4 && ppm_in_chcnt <= 16) {
      for (int i = 0; i < MIN(ppm_in_chcnt, 16); i++) {
        channel_data[i] = ppm_in_chans[i];
      }
    }

    // 3) Set all incoming SBUS values
    static float sbustimer = TrackerSettings::SBUS_ACTIVE_TIME;
    static bool lostmsgsent = false;
    static bool recmsgsent = false;
    sbustimer += (float)CALCULATE_PERIOD / 1000000.0;
    if (SBUS_Read_Data(sbus_in_chans)) {  // Valid SBUS packet received?
      sbustimer = 0;
    }
    // SBUS not received within XX time, disable
    if (sbustimer > TrackerSettings::SBUS_ACTIVE_TIME) {
      for (int i = 0; i < 16; i++) sbus_in_chans[i] = 0;
      if (!lostmsgsent) {
        LOGE("SBUS Data Lost");
        lostmsgsent = true;
      }
      recmsgsent = false;
      // SBUS data still valid, set the channel values to the last SBUS
    } else {
      for (int i = 0; i < 16; i++) channel_data[i] = sbus_in_chans[i];
      if (!recmsgsent) {
        LOGD("SBUS Data Received");
        recmsgsent = true;
      }
      lostmsgsent = false;
    }

    // 4) Set all incoming BT values
    // Bluetooth cannot send a zero value for a channel with PARA. Radios see this as invalid data.
    // So, if the data is coming from a BLE head unit it also has a characteristic to nofity which
    // ones are valid alloww PPM/SBUS pass through on the head or remote boards on ch 1-8
    // If the data is coming from a PARA radio all 8ch's are going to have values, all PPM/SBUS
    // inputs 1-8 will be overridden

    for (int i = 0; i < BT_CHANNELS; i++)
      bt_chans[i] = 0;  // Reset all BT in channels to Zero (Not active)
    for (int i = 0; i < BT_CHANNELS; i++) {
      uint16_t btvalue = BTGetChannel(i);
      if (btvalue > 0) {
        bt_chans[i] = btvalue;
        channel_data[i] = btvalue;
      }
    }

    // 5) If selected input channel went > 1800us reset the center
    // wait for it to drop below 1700 before allowing another reset
    /*int rstppmch = trkset.resetCntPPM() - 1;
    static bool hasrstppm=false;
    if(rstppmch >= 0 && rstppmch < 16) {
        if(channel_data[rstppmch] > 1800 && hasrstppm == false) {
            LOGI("Reset Center - Input Channel %d > 1800us", rstppmch+1);
            pressButton();
            hasrstppm = true;
        } else if (channel_data[rstppmch] < 1700 && hasrstppm == true) {
            hasrstppm = false;
        }
    }*/ //REMOVED as of V2.1

    // 6) Set Auxiliary Functions
    int aux0ch = trkset.auxFunc0Ch();
    int aux1ch = trkset.auxFunc1Ch();
    int aux2ch = trkset.auxFunc2Ch();
    if (aux0ch > 0 || aux1ch > 0) {
      buildAuxData();
      if (aux0ch > 0) channel_data[aux0ch - 1] = auxdata[trkset.auxFunc0()];
      if (aux1ch > 0) channel_data[aux1ch - 1] = auxdata[trkset.auxFunc1()];
      if (aux2ch > 0) channel_data[aux2ch - 1] = auxdata[trkset.auxFunc2()];
    }

    // 7) Set Analog Channels
    if (trkset.analog4Ch() > 0) {
      float an4 = SF1eFilterDo(anFilter[0], analogRead(AN4));
      an4 *= trkset.analog4Gain();
      an4 += trkset.analog4Offset();
      an4 += TrackerSettings::MIN_PWM;
      an4 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an4));
      channel_data[trkset.analog4Ch() - 1] = an4;
    }
    if (trkset.analog5Ch() > 0) {
      float an5 = SF1eFilterDo(anFilter[1], analogRead(AN5));
      an5 *= trkset.analog5Gain();
      an5 += trkset.analog5Offset();
      an5 += TrackerSettings::MIN_PWM;
      an5 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an5));
      channel_data[trkset.analog5Ch() - 1] = an5;
    }
    if (trkset.analog6Ch() > 0) {
      float an6 = SF1eFilterDo(anFilter[2], analogRead(AN6));
      an6 *= trkset.analog6Gain();
      an6 += trkset.analog6Offset();
      an6 += TrackerSettings::MIN_PWM;
      an6 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an6));
      channel_data[trkset.analog6Ch() - 1] = an6;
    }
    if (trkset.analog7Ch() > 0) {
      float an7 = SF1eFilterDo(anFilter[3], analogRead(AN7));
      an7 *= trkset.analog7Gain();
      an7 += trkset.analog7Offset();
      an7 += TrackerSettings::MIN_PWM;
      an7 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an7));
      channel_data[trkset.analog7Ch() - 1] = an7;
    }

    // 8) First decide if 'reset center' pulse should be sent

    static float pulsetimer = 0;
    static bool sendingresetpulse = false;
    int alertch = trkset.alertCh();
    if (alertch > 0) {
      // Synthesize a pulse indicating reset center started
      channel_data[alertch - 1] = TrackerSettings::MIN_PWM;
      if (butdnw) {
        sendingresetpulse = true;
        pulsetimer = 0;
      }
      if (sendingresetpulse) {
        channel_data[alertch - 1] = TrackerSettings::MAX_PWM;
        pulsetimer += (float)CALCULATE_PERIOD / 1000000.0;
        if (pulsetimer > TrackerSettings::RECENTER_PULSE_DURATION) {
          sendingresetpulse = false;
        }
      }
    }

    // 9) Then, set Tilt/Roll/Pan Channel Values (after reset center in case of channel overlap)

    // If the long press for enable/disable isn't set or if there is no reset button configured
    //   always enable the T/R/P outputs
    static bool lastbutmode = false;
    bool buttonpresmode = trkset.buttonPressMode();
    if (buttonpresmode == false || trkset.buttonPin() == 0) trpOutputEnabled = true;

    // On user enabling the button press mode in the GUI default to TRP output off.
    if (lastbutmode == false && buttonpresmode == true) {
      trpOutputEnabled = false;
    }
    lastbutmode = buttonpresmode;

    // If gyro isn't calibrated, don't output TRP
    if (!gyro_calibrated) trpOutputEnabled = false;

    int tltch = trkset.tiltCh();
    int rllch = trkset.rollCh();
    int panch = trkset.panCh();
    if (tltch > 0)
      channel_data[tltch - 1] = trpOutputEnabled == true ? tiltout_ui : trkset.Tlt_cnt();
    if (rllch > 0)
      channel_data[rllch - 1] = trpOutputEnabled == true ? rollout_ui : trkset.Rll_cnt();
    if (panch > 0)
      channel_data[panch - 1] = trpOutputEnabled == true ? panout_ui : trkset.Pan_cnt();

    // 10) Set the PPM Outputs
    for (int i = 0; i < PpmOut_getChnCount(); i++) {
      uint16_t ppmout = channel_data[i];
      if (ppmout == 0) ppmout = TrackerSettings::PPM_CENTER;
      PpmOut_setChannel(i, ppmout);
    }

    // 11) Set all the BT Channels, send the zeros don't center
    bool bleconnected = BTGetConnected();
    trkset.setBLEAddress(BTGetAddress());
    for (int i = 0; i < BT_CHANNELS; i++) {
      BTSetChannel(i, channel_data[i]);
    }

    // 12) Set all SBUS output channels, if disabled set to center
    uint16_t sbus_data[16];
    for (int i = 0; i < 16; i++) {
      uint16_t sbusout = channel_data[i];
      if (sbusout == 0) sbusout = TrackerSettings::PPM_CENTER;
      sbus_data[i] = (static_cast<float>(sbusout) - TrackerSettings::PPM_CENTER) *
                         TrackerSettings::SBUS_SCALE +
                     TrackerSettings::SBUS_CENTER;
    }
    SBUS_TX_BuildData(sbus_data);

    // 13) Set PWM Channels
    for (int i = 0; i < 4; i++) {
      int pwmch = trkset.PWMCh(i) - 1;
      if (pwmch >= 0 && pwmch < 16) {
        uint16_t pwmout = channel_data[pwmch];
        if (pwmout == 0) pwmout = TrackerSettings::PPM_CENTER;
        setPWMValue(i, pwmout);
      }
    }

    // 14 Set USB Joystick Channels, Only 8 channels
    static int joycnt = 0;
    if (joycnt++ == 1) {
      set_JoystickChannels(channel_data);
      joycnt = 0;
    }

    // Update the settings for the GUI
    // Both data and sensor threads will use this data. If data thread has it locked skip this
    // reading.
    if (k_mutex_lock(&data_mutex, K_NO_WAIT) == 0) {
      // Raw values for calibration
      trkset.setRawAccel(raccx, raccy, raccz);
      trkset.setRawGyro(rgyrx, rgyry, rgyrz);
      trkset.setRawMag(rmagx, rmagy, rmagz);

      // Offset values for debug
      trkset.setOffAccel(accx, accy, accz);
      trkset.setOffGyro(gyrx, gyry, gyrz);
      trkset.setOffMag(magx, magy, magz);

      trkset.setRawOrient(tilt, roll, pan);
      trkset.setOffOrient(tilt - tiltoffset, roll - rolloffset,
                          normalize(pan - panoffset, -180, 180));
      trkset.setPPMOut(tiltout_ui, rollout_ui, panout_ui);

      // PPM Input Values
      trkset.setPPMInValues(ppm_in_chans);
      trkset.setBLEValues(bt_chans);
      trkset.setSBUSValues(sbus_in_chans);
      trkset.setChannelOutValues(channel_data);
      trkset.setTRPEnabled(trpOutputEnabled);

      trkset.setGyroCalibrated(gyro_calibrated);

      // Qauterion Data
      float *qd = madgwick.getQuat();
      trkset.setQuaternion(qd);

      // Bluetooth connected
      trkset.setBlueToothConnected(bleconnected);
      k_mutex_unlock(&data_mutex);
    }

    // Adjust sleep for a more accurate period
    usduration = micros64() - usduration;
    if (CALCULATE_PERIOD - usduration <
        CALCULATE_PERIOD * 0.7) {  // Took a long time. Will crash if sleep is too short

      rt_sleep_us(CALCULATE_PERIOD);
    } else {
      rt_sleep_us(CALCULATE_PERIOD - usduration);
    }

#if defined(DEBUG_SENSOR_RATES)
    static int mcount = 0;
    static int64_t mmic = millis64() + 1000;
    if (mmic < millis64()) {  // Every Second
      mmic = millis64() + 1000;
      LOGI("Calc Rate = %d", mcount);
      mcount = 0;
    }
    mcount++;
#endif
  }
}

//----------------------------------------------------------------------
// Sensor Reading Thread
//----------------------------------------------------------------------

void sensor_Thread()
{
  // Gyro Calibration
  float avg[3] = {0, 0, 0};
  float lavg[3] = {0, 0, 0};
  bool initrun = true;
  int passcount = GYRO_STABLE_SAMPLES;

  while (1) {
    rt_sleep_us(SENSOR_PERIOD);

    if (!senseTreadRun) {
      continue;
    }

    // Reset Center on Proximity, Don't need to update this often
    static int sensecount = 0;
    static int minproximity = 100;  // Keeps smallest proximity read.
    static int maxproximity = 0;    // Keeps largest proximity value read.
    if (blesenseboard && sensecount++ == 10) {
      sensecount = 0;
      if (trkset.resetOnWave()) {
        // Reset on Proximity
        if (APDS.proximityAvailable()) {
          int proximity = APDS.readProximity();

          LOGT("Prox=%d", proximity);

          // Store High and Low Values, Generate reset thresholds
          maxproximity = MAX(proximity, maxproximity);
          minproximity = MIN(proximity, minproximity);
          int lowthreshold = minproximity + APDS_HYSTERISIS;
          int highthreshold = maxproximity - APDS_HYSTERISIS;

          // Don't allow reset if high and low thresholds are too close
          if (highthreshold - lowthreshold > APDS_HYSTERISIS * 2) {
            if (proximity < lowthreshold && lastproximity == false) {
              pressButton();
              LOGI("Reset center from a close proximity");
              lastproximity = true;
            } else if (proximity > highthreshold) {
              // Clear flag on proximity clear
              lastproximity = false;
            }
          }
        }
      }
    }

    // Setup Rotations
    float rotation[3];
    trkset.orientRotations(rotation);

    // Accelerometer
    if (IMU.accelerationAvailable()) {
#if defined(DEBUG_SENSOR_RATES)
      static int acount = 0;
      static int64_t mic = millis64() + 1000;
      if (mic < millis64()) {  // Every Second
        mic = millis64() + 1000;
        LOGI("ACC Rate = %d", acount);
        acount = 0;
      }
      acount++;
#endif

      IMU.readRawAccel(raccx, raccy, raccz);
      raccx *= -1.0;  // Flip X to make classic cartesian (+X Right, +Y Up, +Z Vert)
      trkset.accOffset(accxoff, accyoff, acczoff);

      k_mutex_lock(&sensor_mutex, K_FOREVER);

      accx = raccx - accxoff;
      accy = raccy - accyoff;
      accz = raccz - acczoff;

      // Apply Rotation
      float tmpacc[3] = {accx, accy, accz};
      rotate(tmpacc, rotation);
      accx = tmpacc[0];
      accy = tmpacc[1];
      accz = tmpacc[2];

      // For intial orientation setup
      madgsensbits |= MADGINIT_ACCEL;

      k_mutex_unlock(&sensor_mutex);
    }

    // Gyrometer
    if (IMU.gyroscopeAvailable()) {
#if defined(DEBUG_SENSOR_RATES)
      static int gcount = 0;
      static int64_t gmic = millis64() + 1000;
      if (gmic < millis64()) {  // Every Second
        gmic = millis64() + 1000;
        LOGI("GYR Rate = %d", gcount);
        gcount = 0;
      }
      gcount++;
#endif

      IMU.readRawGyro(rgyrx, rgyry, rgyrz);
      rgyrx *= -1.0;  // Flip X to match other sensors

      if (!gyro_calibrated) {
        if (initrun) {  // Preload on first read
          avg[0] = rgyrx;
          avg[1] = rgyry;
          avg[2] = rgyrz;
          lavg[0] = rgyrx;
          lavg[1] = rgyry;
          lavg[2] = rgyrz;
          initrun = false;
        } else {
          avg[0] = (avg[0] * GYRO_LP_BETA) + (rgyrx * (1.0 - GYRO_LP_BETA));
          avg[1] = (avg[1] * GYRO_LP_BETA) + (rgyry * (1.0 - GYRO_LP_BETA));
          avg[2] = (avg[2] * GYRO_LP_BETA) + (rgyrz * (1.0 - GYRO_LP_BETA));

          // Calculate differential of signal
          float diff[3];
          static int64_t lastUpdate = 0;
          int64_t now = micros64();

          float deltat = ((now - lastUpdate) / 1000000.0f);  // set integration time by time elapsed
                                                             // since last filter update
          lastUpdate = now;

          for (int i = 0; i < 3; i++) {
            diff[i] = fabs(avg[i] - lavg[i]) / deltat;
            lavg[i] = avg[i];
          }

          // If rate of change low then decrement the counter
          if (diff[0] < GYRO_PASS_DIFF && diff[1] < GYRO_PASS_DIFF && diff[2] < GYRO_PASS_DIFF)
            passcount--;
          // Otherwise start over
          else {
            passcount = GYRO_STABLE_SAMPLES;
            initrun = true;
          }

          // If enough samples taken at low motion, Success
          if (passcount == 0) {
            trkset.setGyroOffset(avg[0], avg[1], avg[2]);
            clearLEDFlag(LED_GYROCAL);
            gyro_calibrated = true;
          }
        }
      } else {
        trkset.gyroOffset(gyrxoff, gyryoff, gyrzoff);

        k_mutex_lock(&sensor_mutex, K_FOREVER);

        gyrx = rgyrx - gyrxoff;
        gyry = rgyry - gyryoff;
        gyrz = rgyrz - gyrzoff;

        // Apply Rotation
        float tmpgyr[3] = {gyrx, gyry, gyrz};
        rotate(tmpgyr, rotation);
        gyrx = tmpgyr[0];
        gyry = tmpgyr[1];
        gyrz = tmpgyr[2];

        k_mutex_unlock(&sensor_mutex);
      }
    }

    // Magnetometer
    if (IMU.magneticFieldAvailable()) {
#if defined(DEBUG_SENSOR_RATES)
      static int mcount = 0;
      static int64_t mmic = millis64() + 1000;
      if (mmic < millis64()) {  // Every Second
        mmic = millis64() + 1000;
        LOGI("MAG Rate = %d", mcount);
        mcount = 0;
      }
      mcount++;
#endif

      IMU.readRawMagnet(rmagx, rmagy, rmagz);
      // On first read set the min/max values to this reading
      // Get Offsets + Soft Iron Offesets
      float magsioff[9];
      trkset.magOffset(magxoff, magyoff, magzoff);
      trkset.magSiOffset(magsioff);

      k_mutex_lock(&sensor_mutex, K_FOREVER);

      // Calibrate Hard Iron Offsets
      magx = rmagx - magxoff;
      magy = rmagy - magyoff;
      magz = rmagz - magzoff;

      magx = (magx * magsioff[0]) + (magy * magsioff[1]) + (magz * magsioff[2]);
      magy = (magx * magsioff[3]) + (magy * magsioff[4]) + (magz * magsioff[5]);
      magz = (magx * magsioff[6]) + (magy * magsioff[7]) + (magz * magsioff[8]);

      // Apply Rotation
      float tmpmag[3] = {magx, magy, magz};
      rotate(tmpmag, rotation);
      magx = tmpmag[0];
      magy = tmpmag[1];
      magz = tmpmag[2];

      // For inital orientation setup
      madgsensbits |= MADGINIT_MAG;

      k_mutex_unlock(&sensor_mutex);
    }
  }  // END THREAD
}

// FROM https://stackoverflow.com/questions/1628386/normalise-orientation-between-0-and-360
// Normalizes any number to an arbitrary range
// by assuming the range wraps around when going below min or above max
float normalize(const float value, const float start, const float end)
{
  const float width = end - start;          //
  const float offsetValue = value - start;  // value relative to 0

  return (offsetValue - (floor(offsetValue / width) * width)) + start;
  // + start to reset back to start of original range
}

// Rotate, in Order X -> Y -> Z

void rotate(float pn[3], const float rotation[3])
{
  float rot[3] = {0, 0, 0};
  memcpy(rot, rotation, sizeof(rot[0]) * 3);

  // Passed in Degrees
  rot[0] *= DEG_TO_RAD;
  rot[1] *= DEG_TO_RAD;
  rot[2] *= DEG_TO_RAD;

  float out[3];

  // X Rotation
  if (rotation[0] != 0) {
    out[0] = pn[0] * 1 + pn[1] * 0 + pn[2] * 0;
    out[1] = pn[0] * 0 + pn[1] * cos(rot[0]) - pn[2] * sin(rot[0]);
    out[2] = pn[0] * 0 + pn[1] * sin(rot[0]) + pn[2] * cos(rot[0]);
    memcpy(pn, out, sizeof(out[0]) * 3);
  }

  // Y Rotation
  if (rotation[1] != 0) {
    out[0] = pn[0] * cos(rot[1]) - pn[1] * 0 + pn[2] * sin(rot[1]);
    out[1] = pn[0] * 0 + pn[1] * 1 + pn[2] * 0;
    out[2] = -pn[0] * sin(rot[1]) + pn[1] * 0 + pn[2] * cos(rot[1]);
    memcpy(pn, out, sizeof(out[0]) * 3);
  }

  // Z Rotation
  if (rotation[2] != 0) {
    out[0] = pn[0] * cos(rot[2]) - pn[1] * sin(rot[2]) + pn[2] * 0;
    out[1] = pn[0] * sin(rot[2]) + pn[1] * cos(rot[2]) + pn[2] * 0;
    out[2] = pn[0] * 0 + pn[1] * 0 + pn[2] * 1;
    memcpy(pn, out, sizeof(out[0]) * 3);
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
  aacc[0] = 0;
  aacc[1] = 0;
  aacc[2] = 0;
  amag[0] = 0;
  amag[1] = 0;
  amag[2] = 0;
  LOGI("Resetting fusion algorithm");
}

/* Builds data for auxiliary functions
 */

void buildAuxData()
{
  float pwmrange = (TrackerSettings::MAX_PWM - TrackerSettings::MIN_PWM);
  auxdata[TrackerSettings::AUX_GYRX] = (gyrx / 1000) * pwmrange + TrackerSettings::PPM_CENTER;
  auxdata[TrackerSettings::AUX_GYRY] = (gyry / 1000) * pwmrange + TrackerSettings::PPM_CENTER;
  auxdata[TrackerSettings::AUX_GYRZ] = (gyrz / 1000) * pwmrange + TrackerSettings::PPM_CENTER;
  auxdata[TrackerSettings::AUX_ACCELX] = (accx / 2.0f) * pwmrange + TrackerSettings::PPM_CENTER;
  auxdata[TrackerSettings::AUX_ACCELY] = (accy / 2.0f) * pwmrange + TrackerSettings::PPM_CENTER;
  auxdata[TrackerSettings::AUX_ACCELZ] = (accz / 1.0f) * pwmrange + TrackerSettings::PPM_CENTER;
  auxdata[TrackerSettings::AUX_ACCELZO] =
      ((accz - 1.0f) / 2.0f) * pwmrange + TrackerSettings::PPM_CENTER;
  auxdata[TrackerSettings::BT_RSSI] =
      static_cast<float>(BTGetRSSI()) / 127.0 * pwmrange + TrackerSettings::MIN_PWM;
}