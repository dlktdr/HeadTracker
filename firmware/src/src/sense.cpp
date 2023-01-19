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
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <zephyr.h>

#include "MadgwickAHRS/MadgwickAHRS.h"
#include "analog.h"
#include "ble.h"
#include "defines.h"
#include "filters.h"
#include "filters/SF1eFilter.h"
#include "io.h"
#include "joystick.h"
#include "log.h"
#include "nano33ble.h"
#include "pmw.h"
#include "soc_flash.h"
#include "trackersettings.h"
#include "uart_mode.h"

#if defined(HAS_APDS9960)
#include "APDS9960/APDS9960.h"
#endif
#if defined(HAS_LSM9DS1)
#include "LSM9DS1/LSM9DS1.h"
#endif
#if defined(HAS_MPU6500)
#include "MPU6xxx/inv_mpu.h"
#endif
#if defined(HAS_QMC5883)
#include "QMC5883/qmc5883.h"
#endif
#if defined(HAS_BMI270)
#include "BMI270/bmi270.h"
#include "BMI270/bmi2common.h"
#endif
#if defined(HAS_BMM150)
#include "BMM150/bmm150.h"
#include "BMM150/bmm150_common.h"
#endif

//#define DEBUG_SENSOR_RATES

void gyroCalibrate();

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
static bool trpOutputEnabled = false;  // Default to disabled T/R/P output

// Input Channel Data
static uint16_t ppm_in_chans[16];
static uint16_t uart_in_chans[16];
static uint16_t bt_chans[TrackerSettings::BT_CHANNELS];
static float bt_chansf[TrackerSettings::BT_CHANNELS];

// Output channel data
static uint16_t channel_data[16];

Madgwick madgwick;

int64_t usduration = 0;
int64_t senseUsDuration = 0;

#if defined(HAS_APDS9960)
static bool blesenseboard = false;
static bool lastproximity = false;
#endif

#if defined(HAS_LSM9DS1)
LSM9DS1Class IMU;
#endif

#if defined(HAS_BMI270)
struct bmi2_dev bmi2_dev;
#endif

#if defined(HAS_BMM150)
struct bmm150_dev bmm1_dev;
#endif

#define SENSOR_VALUE_TO_FLOAT(x) ((float)x.val1 + (float)x.val2 / 1000000.0f)

// Initial Orientation Data+Vars
#define MADGINIT_ACCEL 0x01
#define MADGINIT_MAG 0x02
#define MADGINIT_READY (MADGINIT_ACCEL | MADGINIT_MAG)

K_MUTEX_DEFINE(sensor_mutex);

static int madgreads = 0;
static uint8_t madgsensbits = 0;
static volatile bool firstrun = true;
static float aacc[3] = {0, 0, 0};
static float amag[3] = {0, 0, 0};

// Analog Filters
SF1eFilter *anFilter[AN_CH_CNT];

static struct k_poll_signal senseThreadRunSignal = K_POLL_SIGNAL_INITIALIZER(senseThreadRunSignal);
struct k_poll_event senseRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &senseThreadRunSignal),
};

static struct k_poll_signal calculateThreadRunSignal =
    K_POLL_SIGNAL_INITIALIZER(calculateThreadRunSignal);
struct k_poll_event calculateRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
                             &calculateThreadRunSignal),
};

int sense_Init()
{
  const struct device *i2c_dev = device_get_binding("I2C_1");
  if (!i2c_dev) {
    LOGE("Could not get device binding for I2C");
    return false;
  }

  i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_MASTER);

#if defined(HAS_LSM9DS1)
  if (!IMU.begin()) {
    LOGE("Failed to initalize sensors");
    return -1;
  }
#endif

#if defined(HAS_BMI270)
  int8_t rslt;
  uint8_t sensor_list[2] = {BMI2_ACCEL, BMI2_GYRO};
  rslt = bmi2_interface_init(&bmi2_dev, BMI2_I2C_INTF);
  bmi2_error_codes_print_result(rslt);

  /* Initialize bmi270. */
  rslt = bmi270_init(&bmi2_dev);
  bmi2_error_codes_print_result(rslt);

  if (rslt == BMI2_OK) {
    /* Accel and gyro configuration settings. */
    rslt = set_accel_gyro_config(&bmi2_dev);
    bmi2_error_codes_print_result(rslt);

    if (rslt == BMI2_OK) {
      /* NOTE:
       * Accel and Gyro enable must be done after setting configurations
       */
      rslt = bmi2_sensor_enable(sensor_list, 2, &bmi2_dev);
      bmi2_error_codes_print_result(rslt);
    }
  } else
    return -1;

#endif

#if defined(HAS_BMM150)
  /* Status of api are returned to this variable */
  int8_t rbslt;

  rbslt = bmm150_interface_selection(&bmm1_dev);
  bmm150_error_codes_print_result("bmm150_interface_selection", rbslt);
  if (rbslt == BMM150_OK) {
    rbslt = bmm150_init(&bmm1_dev);
    bmm150_error_codes_print_result("bmm150_init", rbslt);

    if (rbslt == BMM150_OK) {
      rbslt = set_config(&bmm1_dev);
      bmm150_error_codes_print_result("set_config", rbslt);
    }
  }

#endif

#if defined(HAS_MPU6500)
  mpu_select_device(0);
  mpu_init_structures();
  mpu_init(NULL);
  mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  mpu_set_gyro_fsr(2000);
  mpu_set_accel_fsr(2);
  mpu_set_sample_rate(140);
  mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
#endif

#if defined(HAS_APDS9960)
  // Initalize Gesture Sensor
  if (!APDS.begin()) {
    blesenseboard = false;
    trkset.setDataisSense(false);
  } else {
    blesenseboard = true;
    trkset.setDataisSense(true);
  }
#endif

#if defined(HAS_QMC5883)
  qmc5883Init();
#endif

  for (int i = 0; i < TrackerSettings::BT_CHANNELS; i++) {
    bt_chansf[i] = 0;
  }

  // Create analog filters
  for (int i = 0; i < AN_CH_CNT; i++) {
    anFilter[i] = SF1eFilterCreate(AN_FILT_FREQ, AN_FILT_MINCO, AN_FILT_SLOPE, AN_FILT_DERCO);
    SF1eFilterInit(anFilter[i]);
  }

  // Start reading the IMU sensors + fusion algorithm
  k_poll_signal_raise(&senseThreadRunSignal, 1);

  // Start doing the other calculations
  k_poll_signal_raise(&calculateThreadRunSignal, 1);

  return 0;
}

//----------------------------------------------------------------------
// Calculations and Main Channel Thread
//----------------------------------------------------------------------

void calculate_Thread()
{
  while (1) {
    // Do not execute below until after initialization has happened
    k_poll(calculateRunEvents, 1, K_FOREVER);

    if (pauseForFlash) {
      rt_sleep_ms(10);
      continue;
    }

    usduration = micros64();

    // Use a mutex so sensor data can't be updated part way
    k_mutex_lock(&sensor_mutex, K_FOREVER);
    roll = madgwick.getPitch();
    tilt = madgwick.getRoll();
    pan = madgwick.getYaw();
    k_mutex_unlock(&sensor_mutex);

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
        (tilt - tiltoffset) * trkset.getTlt_Gain() * (trkset.isTiltReversed() ? -1.0 : 1.0);
    uint16_t tiltout_ui = tiltout + trkset.getTlt_Cnt();  // Apply Center Offset
    tiltout_ui = MAX(MIN(tiltout_ui, trkset.getTlt_Max()), trkset.getTlt_Min());  // Limit Output

    // Roll output
    float rollout =
        (roll - rolloffset) * trkset.getRll_Gain() * (trkset.isRollReversed() ? -1.0 : 1.0);
    uint16_t rollout_ui = rollout + trkset.getRll_Cnt();  // Apply Center Offset
    rollout_ui = MAX(MIN(rollout_ui, trkset.getRll_Max()), trkset.getRll_Min());  // Limit Output

    // Pan output, Normalize to +/- 180 Degrees
    float panout = normalize((pan - panoffset), -180, 180) * trkset.getPan_Gain() *
                   (trkset.isPanReversed() ? -1.0 : 1.0);
    uint16_t panout_ui = panout + trkset.getPan_Cnt();  // Apply Center Offset
    panout_ui = MAX(MIN(panout_ui, trkset.getPan_Max()), trkset.getPan_Min());  // Limit Output

    // Reset on tilt
    static bool doresetontilt = false;
    if (trkset.getRstOnTlt()) {
      static bool tiltpeak = false;
      static float resettime = 0.0f;
      enum {
        HITNONE,
        HITMIN,
        HITMAX,
      };
      static int minmax = HITNONE;
      if (rollout_ui == trkset.getRll_Max()) {
        if (tiltpeak == false && minmax == HITNONE) {
          tiltpeak = true;
          minmax = HITMAX;
        } else if (minmax == HITMIN) {
          minmax = HITNONE;
          tiltpeak = false;
          doresetontilt = true;
        }

      } else if (rollout_ui == trkset.getRll_Min()) {
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

    // 3) Set all incoming UART values (Sbus/Crsf)
    bool isUartValid = UartGetChannels(uart_in_chans);
    static bool lostmsgsent = false;
    static bool recmsgsent = false;
    if (!isUartValid) {
      if (!lostmsgsent) {
        LOGE("Uart(SBUS/CRSF) Data Lost");
        lostmsgsent = true;
      }
      recmsgsent = false;
      // SBUS data still valid, set the channel values to the last SBUS
    } else {
      for (int i = 0; i < 16; i++) {
        channel_data[i] = uart_in_chans[i];
      }
      if (!recmsgsent) {
        LOGD("Uart(SBUS/CRSF) Data Received");
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

    for (int i = 0; i < TrackerSettings::BT_CHANNELS; i++)
      bt_chans[i] = 0;  // Reset all BT in channels to Zero (Not active)
    for (int i = 0; i < TrackerSettings::BT_CHANNELS; i++) {
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
    int aux0ch = trkset.getAux0Ch();
    int aux1ch = trkset.getAux1Ch();
    int aux2ch = trkset.getAux2Ch();
    if (aux0ch > 0 || aux1ch > 0 || aux2ch > 0) {
      buildAuxData();
      if (aux0ch > 0) channel_data[aux0ch - 1] = auxdata[trkset.getAux0Func()];
      if (aux1ch > 0) channel_data[aux1ch - 1] = auxdata[trkset.getAux1Func()];
      if (aux2ch > 0) channel_data[aux2ch - 1] = auxdata[trkset.getAux2Func()];
    }

    // 7) Set Analog Channels
#ifdef AN0
    if (trkset.getAn0Ch() > 0) {
      float an4 = SF1eFilterDo(anFilter[0], analogRead(AN0));
      an4 *= trkset.getAn0Gain();
      an4 += trkset.getAn0Off();
      an4 += TrackerSettings::MIN_PWM;
      an4 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an4));
      channel_data[trkset.getAn0Ch() - 1] = an4;
    }
#endif
#ifdef AN1
    if (trkset.getAn1Ch() > 0) {
      float an5 = SF1eFilterDo(anFilter[1], analogRead(AN1));
      an5 *= trkset.getAn1Gain();
      an5 += trkset.getAn1Off();
      an5 += TrackerSettings::MIN_PWM;
      an5 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an5));
      channel_data[trkset.getAn1Ch() - 1] = an5;
    }
#endif
#ifdef AN2
    if (trkset.getAn2Ch() > 0) {
      float an6 = SF1eFilterDo(anFilter[2], analogRead(AN2));
      an6 *= trkset.getAn2Gain();
      an6 += trkset.getAn2Off();
      an6 += TrackerSettings::MIN_PWM;
      an6 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an6));
      channel_data[trkset.getAn2Ch() - 1] = an6;
    }
#endif
#ifdef AN3
    if (trkset.getAn3Ch() > 0) {
      float an7 = SF1eFilterDo(anFilter[3], analogRead(AN3));
      an7 *= trkset.getAn3Gain();
      an7 += trkset.getAn3Off();
      an7 += TrackerSettings::MIN_PWM;
      an7 = MAX(TrackerSettings::MIN_PWM, MIN(TrackerSettings::MAX_PWM, an7));
      channel_data[trkset.getAn3Ch() - 1] = an7;
    }
#endif

    // 8) First decide if 'reset center' pulse should be sent
    static float pulsetimer = 0;
    static bool sendingresetpulse = false;
    int alertch = trkset.getAlertCh();
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
    bool buttonpresmode = trkset.getButLngPs();
    if (buttonpresmode == false || trkset.getButtonPin() == 0) trpOutputEnabled = true;

    // On user enabling the button press mode in the GUI default to TRP output off.
    if (lastbutmode == false && buttonpresmode == true) {
      trpOutputEnabled = false;
    }
    lastbutmode = buttonpresmode;

    int tltch = trkset.getTltCh();
    int rllch = trkset.getRllCh();
    int panch = trkset.getPanCh();
    if (tltch > 0)
      channel_data[tltch - 1] = trpOutputEnabled == true ? tiltout_ui : trkset.getTlt_Cnt();
    if (rllch > 0)
      channel_data[rllch - 1] = trpOutputEnabled == true ? rollout_ui : trkset.getRll_Cnt();
    if (panch > 0)
      channel_data[panch - 1] = trpOutputEnabled == true ? panout_ui : trkset.getPan_Cnt();

    // If uart output set to CRSF_OUT, force channel 5 (AUX1/ARM) to high, will override all other
    // channels
    if (trkset.getUartMode() == TrackerSettings::UART_MODE_CRSFOUT) {
      if (trkset.getCh5Arm()) channel_data[4] = 2000;
    }

    // 10) Set the PPM Outputs
    PpmOut_execute();
    for (int i = 0; i < PpmOut_getChnCount(); i++) {
      uint16_t ppmout = channel_data[i];
      if (ppmout == 0) ppmout = TrackerSettings::PPM_CENTER;
      PpmOut_setChannel(i, ppmout);
    }

    // 11) Set all the BT Channels, send the zeros don't center
    bool bleconnected = BTGetConnected();
    trkset.setDataBtAddr(BTGetAddress());
    for (int i = 0; i < TrackerSettings::BT_CHANNELS; i++) {
      BTSetChannel(i, channel_data[i]);
    }

    // 12) Set all UART output channels, if disabled(0) set to center
    uint16_t uart_data[16];
    for (int i = 0; i < 16; i++) {
      if (channel_data[i] == 0)
        uart_data[i] = TrackerSettings::PPM_CENTER;
      else
        uart_data[i] = channel_data[i];
    }
    UartSetChannels(uart_data);

    // 13) Set PWM Channels
    int8_t pwmchs[4] = {trkset.getPwm0(), trkset.getPwm1(), trkset.getPwm2(), trkset.getPwm3()};
    for (int i = 0; i < 4; i++) {
      int pwmch = pwmchs[i] - 1;
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
    // Serial also uses this data, make sure writes are complete.
    //  If data thread has it locked just skip this reading
    if (k_mutex_lock(&data_mutex, K_NO_WAIT) == 0) {
      // Raw values for calibration
      trkset.setDataAccX(raccx);
      trkset.setDataAccY(raccy);
      trkset.setDataAccZ(raccz);

      trkset.setDataGyroX(rgyrx);
      trkset.setDataGyroY(rgyry);
      trkset.setDataGyroZ(rgyrz);

      trkset.setDataMagX(rmagx);
      trkset.setDataMagY(rmagy);
      trkset.setDataMagZ(rmagz);

      trkset.setDataOff_AccX(accx);
      trkset.setDataOff_AccY(accy);
      trkset.setDataOff_AccZ(accz);

      trkset.setDataOff_GyroX(gyrx);
      trkset.setDataOff_GyroY(gyry);
      trkset.setDataOff_GyroZ(gyrz);

      trkset.setDataOff_MagX(magx);
      trkset.setDataOff_MagY(magy);
      trkset.setDataOff_MagZ(magz);

      trkset.setDataTilt(tilt);
      trkset.setDataRoll(roll);
      trkset.setDataPan(pan);

      trkset.setDataTiltOff(tilt - tiltoffset);
      trkset.setDataRollOff(roll - rolloffset);
      trkset.setDataPanOff(normalize(pan - panoffset, -180, 180));

      trkset.setDataTiltOut(tiltout_ui);
      trkset.setDataRollOut(rollout_ui);
      trkset.setDataPanOut(panout_ui);

      // PPM Input Values
      trkset.setDataPpmCh(ppm_in_chans);
      trkset.setDataBtCh(bt_chans);
      trkset.setDataUartCh(uart_in_chans);
      trkset.setDataChOut(channel_data);
      trkset.setDataTrpEnabled(trpOutputEnabled);

      // Qauterion Data
      float *qd = madgwick.getQuat();
      trkset.setDataQuat(qd);

      // Bluetooth connected
      trkset.setDataBtCon(bleconnected);
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
  while (1) {
    // Do not execute below until after initialization has happened
    k_poll(senseRunEvents, 1, K_FOREVER);

    if (pauseForFlash) {
      rt_sleep_ms(10);
      continue;
    }

    senseUsDuration = micros64();

#if defined(HAS_APDS9960)
    // Reset Center on Proximity, Don't need to update this often
    static int sensecount = 0;
    static int minproximity = 100;  // Keeps smallest proximity read.
    static int maxproximity = 0;    // Keeps largest proximity value read.
    if (blesenseboard && sensecount++ == 10) {
      sensecount = 0;
      if (trkset.getRstOnWave()) {
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
#endif

    // Setup Rotations
    float rotation[3] = {trkset.getRotX(), trkset.getRotY(), trkset.getRotZ()};

    // Read the data from the sensors
    float tacc[3], tgyr[3], tmag[3];
    bool accValid = false;
    bool gyrValid = false;
    bool magValid = false;

#if defined(HAS_LSM9DS1)
    if (IMU.accelerationAvailable()) {
      IMU.readRawAccel(tacc[0], tacc[1], tacc[2]);
      tacc[0] *= -1.0;  // Flip X
      accValid = true;
    }
    if (IMU.magneticFieldAvailable()) {
      IMU.readRawMagnet(tmag[0], tmag[1], tmag[2]);
      magValid = true;
    }
    if (IMU.gyroscopeAvailable()) {
      IMU.readRawGyro(tgyr[0], tgyr[1], tgyr[2]);
      tgyr[0] *= -1.0;  // Flip X to match other sensors
      gyrValid = true;
    }
#endif

#if defined(HAS_BMI270)
    int8_t rslt;
    uint16_t int_status = 0;
    struct bmi2_sens_data sensor_data = {{0}};
    rslt = bmi2_get_int_status(&int_status, &bmi2_dev);
    bmi2_error_codes_print_result(rslt);
    /* To check the data ready interrupt status and print the status for 10 samples. */
    if ((int_status & BMI2_ACC_DRDY_INT_MASK) && (int_status & BMI2_GYR_DRDY_INT_MASK)) {
      /* Get accel and gyro data for x, y and z axis. */
      rslt = bmi2_get_sensor_data(&sensor_data, &bmi2_dev);
      bmi2_error_codes_print_result(rslt);

      /* Converting lsb to meter per second squared for 16 bit accelerometer at 2G range. */
      tacc[0] = lsb_to_mps2(sensor_data.acc.y, 2, bmi2_dev.resolution) / GRAVITY_EARTH;
      tacc[1] = -1.0f * lsb_to_mps2(sensor_data.acc.x, 2, bmi2_dev.resolution) / GRAVITY_EARTH;
      tacc[2] = lsb_to_mps2(sensor_data.acc.z, 2, bmi2_dev.resolution) / GRAVITY_EARTH;
      // printk("\nAccX=%4.2f,Y=%4.2f,Z=%4.2f\n", tacc[0], tacc[1], tacc[2]);
      /* Converting lsb to degree per second for 16 bit gyro at 2000dps range. */

      tgyr[0] = lsb_to_dps(sensor_data.gyr.y, 2000, bmi2_dev.resolution);
      tgyr[1] = -1.0f * lsb_to_dps(sensor_data.gyr.x, 2000, bmi2_dev.resolution);
      tgyr[2] = lsb_to_dps(sensor_data.gyr.z, 2000, bmi2_dev.resolution);
      // printk("GyrX=%4.2f,Y=%4.2f,Z=%4.2f\n", tgyr[0], tgyr[1], tgyr[2]);
      accValid = true;
      gyrValid = true;
    }

#endif

#if defined(HAS_BMM150)
    int8_t rbslt;
    struct bmm150_mag_data mag_data;
    rbslt = bmm150_read_mag_data(&mag_data, &bmm1_dev);
    bmm150_error_codes_print_result("bmm150_read_mag_data", rbslt);
    tmag[0] = mag_data.y;
    tmag[1] = mag_data.x;
    tmag[2] = mag_data.z;
    magValid = true;
#endif

#if defined(HAS_QMC5883)
    if (qmc5883Read(tmag)) {
      magValid = true;
    }
#endif

#if defined(HAS_MPU6500)
    // Read MPU6500
    short _gyro[3];
    short _accel[3];
    unsigned long timestamp;
    if (!mpu_get_accel_reg(_accel, &timestamp)) accValid = true;
    unsigned short ascale = 1;
    ;
    mpu_get_accel_sens(&ascale);
    tacc[0] = (float)_accel[0] / (float)ascale;
    tacc[1] = (float)_accel[1] / (float)ascale;
    tacc[2] = (float)_accel[2] / (float)ascale;
    if (!mpu_get_gyro_reg(_gyro, &timestamp)) gyrValid = true;
    float gscale = 1.0f;
    mpu_get_gyro_sens(&gscale);
    tgyr[0] = _gyro[0] / gscale;
    tgyr[1] = _gyro[1] / gscale;
    tgyr[2] = _gyro[2] / gscale;
#endif

    k_mutex_lock(&sensor_mutex, K_FOREVER);

    // -- Accelerometer
    if (accValid) {
      raccx = tacc[0];
      raccy = tacc[1];
      raccz = tacc[2];
      accxoff = trkset.getAccXOff();
      accyoff = trkset.getAccYOff();
      acczoff = trkset.getAccZOff();

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
    }

    // --- Gyrometer Calcs
    if (gyrValid) {
      rgyrx = tgyr[0];
      rgyry = tgyr[1];
      rgyrz = tgyr[2];
      gyrxoff = trkset.getGyrXOff();
      gyryoff = trkset.getGyrYOff();
      gyrzoff = trkset.getGyrZOff();

      gyrx = rgyrx - gyrxoff;
      gyry = rgyry - gyryoff;
      gyrz = rgyrz - gyrzoff;

      // Apply Rotation
      float tmpgyr[3] = {gyrx, gyry, gyrz};
      rotate(tmpgyr, rotation);
      gyrx = tmpgyr[0];
      gyry = tmpgyr[1];
      gyrz = tmpgyr[2];
    }

    if(!trkset.getDisMag()) {
      if (magValid) {
        // --- Magnetometer Calcs
        rmagx = tmag[0];
        rmagy = tmag[1];
        rmagz = tmag[2];
        float magsioff[9];
        magxoff = trkset.getMagXOff();
        magyoff = trkset.getMagYOff();
        magzoff = trkset.getMagZOff();
        trkset.getMagSiOff(magsioff);

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
      }
    } else {
        magx = 0;
        magy = 0;
        magz = 0;
        madgsensbits |= MADGINIT_MAG;
      }

    // Run Gyro Calibration
    gyroCalibrate();

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
      // Period Between Samples
      madgwick.update(gyrx * DEG_TO_RAD, gyry * DEG_TO_RAD, gyrz * DEG_TO_RAD, accx, accy, accz,
                      magx, magy, magz, madgwick.deltatUpdate());
      if (firstrun) {
        panoffset = pan;
        firstrun = false;
      }
    }

    k_mutex_unlock(&sensor_mutex);

    // Adjust sleep for a more accurate period
    senseUsDuration = micros64() - senseUsDuration;
    if (SENSOR_PERIOD - senseUsDuration <
        SENSOR_PERIOD * 0.7) {  // Took a long time. Will crash if sleep is too short

      rt_sleep_us(SENSOR_PERIOD);
    } else {
      rt_sleep_us(SENSOR_PERIOD - senseUsDuration);
    }

#if defined(DEBUG_SENSOR_RATES)
    static int mcount = 0;
    static int64_t mmic = millis64() + 1000;
    if (mmic < millis64()) {  // Every Second
      mmic = millis64() + 1000;
      LOGI("Sense Rate = %d", mcount);
      mcount = 0;
    }
    if (accValid) mcount++;
#endif
  }  // END THREAD
}

void gyroCalibrate()
{
  static float last_gyro_mag = 0;
  static float last_acc_mag = 0;
  static float filt_gyrx = 0;
  static float filt_gyry = 0;
  static float filt_gyrz = 0;
  static bool sent_gyro_cal_msg = false;
  static uint32_t filter_samples = 0;
  static uint64_t lasttime = 0;

  uint64_t time = micros64();
  if (lasttime == 0) {  // Skip first run
    lasttime = time;
    return;
  }
  float deltatime = (float)(time - lasttime) / 1000000.0f;
  if (deltatime == 0.0f) return;
  lasttime = time;

  float gyro_magnitude = sqrt(rgyrx * rgyrx + rgyry * rgyry + rgyrz * rgyrz);
  float acc_magnitude = sqrt(raccx * raccx + raccy * raccy + raccz * raccz);
  float gyro_dif = (gyro_magnitude - last_gyro_mag) / deltatime;
  last_gyro_mag = gyro_magnitude;
  float acc_dif = (acc_magnitude - last_acc_mag) / deltatime;
  last_acc_mag = acc_magnitude;

  // Is Gyro anc Accelerometer stable?
  if (fabs(gyro_dif) < GYRO_STABLE_DIFF && fabs(acc_dif) < ACC_STABLE_DIFF) {
    // First run, preload filter
    if (filter_samples == 0) {
      filt_gyrx = rgyrx;
      filt_gyry = rgyry;
      filt_gyrz = rgyrz;
      sent_gyro_cal_msg = false;
      filter_samples++;
    } else if (filter_samples < GYRO_STABLE_SAMPLES) {
      filt_gyrx = ((1.0f - GYRO_SAMPLE_WEIGHT) * filt_gyrx) + (GYRO_SAMPLE_WEIGHT * rgyrx);
      filt_gyry = ((1.0f - GYRO_SAMPLE_WEIGHT) * filt_gyry) + (GYRO_SAMPLE_WEIGHT * rgyry);
      filt_gyrz = ((1.0f - GYRO_SAMPLE_WEIGHT) * filt_gyrz) + (GYRO_SAMPLE_WEIGHT * rgyrz);
      filter_samples++;
    } else if (filter_samples == GYRO_STABLE_SAMPLES) {
      // Set the new Gyro Offset Values
      k_mutex_lock(&data_mutex, K_FOREVER);
      trkset.setGyrXOff(filt_gyrx);
      trkset.setGyrYOff(filt_gyry);
      trkset.setGyrZOff(filt_gyrz);
      k_mutex_unlock(&data_mutex);

      // Check if they differ from the flash values and save if out of range
      if(fabs(gyrxoff - filt_gyrx) > GYRO_FLASH_IF_OFFSET ||
         fabs(gyryoff - filt_gyry) > GYRO_FLASH_IF_OFFSET ||
         fabs(gyrzoff - filt_gyrz) > GYRO_FLASH_IF_OFFSET) {
        if (!sent_gyro_cal_msg) {
          trkset.saveToEEPROM();
          LOGW("Gyro calibration differs from saved value. Updating flash, x=%.3f,y=%.3f,z=%.3f", filt_gyrx, filt_gyry, filt_gyrz);
          sent_gyro_cal_msg = true;
        }
      }
      filter_samples++;
    }
  } else {
    filter_samples = 0;
  }

  // Output in CSV format for determining limits
  // printk("%.4f,%.2f,%.2f\n", (float)time / 1000000.0f, gyro_dif, acc_dif);
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
  // TODO add a mutex here.
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