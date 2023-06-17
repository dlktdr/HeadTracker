/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2023 Cliff Blackburn
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

#pragma once

#include <stdint.h>
#include "defines.h"

// When adding a new feature, always add at end
enum {
FEAT_WS2812=0,
FEAT_3DIODE_RGB,
FEAT_POWERLED,
FEAT_BUZZER,
FEAT_NOTIFYLED,
FEAT_MPU6500,
FEAT_QMC5883,
FEAT_APDS9960,
FEAT_BMI270,
FEAT_BNO055,
FEAT_MPU9050,
FEAT_BMM150,
FEAT_LSM9DS1,
FEAT_LSM6DS3,
FEAT_CENTERBTN,
FEAT_PPMIN,
FEAT_PPMOUT,
FEAT_PWM,
FEAT_USBHID,
FEAT_UART,
FEAT_UART_INV_REQ_SHORT,
FEAT_BT4,
FEAT_BT5,
FEAT_BTEDR,
FEAT_WIFI,
FEAT_LZ4_COMPRESS
};

uint32_t getFeatures0() { //0-31
  uint32_t features=0;
  #ifdef HAS_WS2812
    features|=1<<FEAT_WS2812;
  #endif
  #ifdef HAS_3DIODE_RGB
    features|=1<<FEAT_3DIODE_RGB;
  #endif
  #ifdef HAS_POWERLED
    features|=1<<FEAT_POWERLED;
  #endif
  #ifdef HAS_BUZZER
    features|=1<<FEAT_BUZZER;
  #endif
  #ifdef HAS_NOTIFYLED
    features|=1<<FEAT_NOTIFYLED;
  #endif
  #ifdef HAS_MPU6500
    features|=1<<FEAT_MPU6500;
  #endif
  #ifdef HAS_QMC5883
    features|=1<<FEAT_QMC5883;
  #endif
  #ifdef HAS_APDS9960
    features|=1<<FEAT_APDS9960;
  #endif
  #ifdef HAS_BMI270
    features|=1<<FEAT_BMI270;
  #endif
  #ifdef HAS_BNO055
    features|=1<<FEAT_BNO055;
  #endif
  #ifdef HAS_MPU9050
    features|=1<<FEAT_MPU9050;
  #endif
  #ifdef HAS_BMM150
    features|=1<<FEAT_BMM150;
  #endif
  #ifdef HAS_LSM9DS1
    features|=1<<FEAT_LSM9DS1;
  #endif
  #ifdef HAS_LSM6DS3
    features|=1<<FEAT_LSM6DS3;
  #endif
  #ifdef HAS_CENTERBTN
    features|=1<<FEAT_CENTERBTN;
  #endif
  #ifdef HAS_PPM
    features|=1<<FEAT_PPMIN;
  #endif
  #ifdef HAS_PPM
    features|=1<<FEAT_PPMOUT;
  #endif
  #ifdef HAS_PWM
    features|=1<<FEAT_PWM;
  #endif
  #ifdef HAS_USBHID
    features|=1<<FEAT_USBHID;
  #endif
  #ifdef HAS_UART
    features|=1<<FEAT_UART;
  #endif
  #ifdef HAS_UART_INV_REQ_SHORT
    features|=1<<FEAT_UART_INV_REQ_SHORT;
  #endif
  #ifdef HAS_BT4
    features|=1<<FEAT_BT4;
  #endif
  #ifdef HAS_BT5
    features|=1<<FEAT_BT5;
  #endif
  #ifdef HAS_BTEDR
    features|=1<<FEAT_BTEDR;
  #endif
  #ifdef HAS_WIFI
    features|=1<<FEAT_WIFI;
  #endif
  return features;
}

uint32_t getFeatures1() // 32-63
{
  uint32_t features=0;
  return features;
}