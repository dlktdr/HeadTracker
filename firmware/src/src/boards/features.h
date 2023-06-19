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
#include "sense.h"

// 2 Character descriptor for features included in firmware
#define FEAT_WS2812             "28" // WS2812 Serial RGB LED
#define FEAT_3DIODE_RGB         "3D" // 3 Individual Diodes for RGB
#define FEAT_POWERLED           "PL" // Power LED
#define FEAT_BUZZER             "BZ" // Buzzer
#define FEAT_NOTIFYLED          "NL" // Notification LED (Single color)
#define FEAT_MPU6500            "60" // MPU6050
#define FEAT_QMC5883            "58" // QMC5883
#define FEAT_APDS9960           "99" // APDS9960
#define FEAT_BMI270             "B2" // BMI270
#define FEAT_MPU9050            "90" // MPU9050
#define FEAT_BMM150             "B1" // BMM150
#define FEAT_LSM9DS1            "9D" // LSM9DS1 9 Axis Sensor
#define FEAT_LSM6DS3            "6D" // LSM6DS3
#define FEAT_MAGNETOMETER       "MA" // PCB has a Magnetometer
#define FEAT_CENTERBTN          "CB" // Center Button
#define FEAT_PPMIN              "PI" // PPM Input *
#define FEAT_PPMOUT             "PO" // PPM Output *
#define FEAT_USBHID             "HD" // USB HID Support (Joystick)
#define FEAT_UART               "UR" // UART port
#define FEAT_UART_INV_REQ_SHORT "US" // Uart requires external short * *
#define FEAT_BT4                "B4" // Bluetooth LE 4
#define FEAT_BT5                "B5" // Bluetooth LE 5
#define FEAT_BTEDR              "BC" // Bluetooth Classic/EDR
#define FEAT_WIFI               "WF" // Wifi
#define FEAT_PWM1               "P1" // PWM Output 1 *
#define FEAT_PWM2               "P2" // PWM Output 2 *
#define FEAT_PWM3               "P3" // PWM Output 3 *
#define FEAT_PWM4               "P4" // PWM Output 4 *
#define FEAT_ANALOG1            "A1" // Analog Input 1 *
#define FEAT_ANALOG2            "A2" // Analog Input 2 *
#define FEAT_ANALOG3            "A3" // Analog Input 3 *
#define FEAT_ANALOG4            "A4" // Analog Input 4 *
#define FEAT_BATTMON            "BM" // Battery Monitor

// Items with a *, need to send the list of pins they use or can use.
// Feature description is then e.g. for Anaolog 1
//   A1S1S2S3S26
// Analog one can be on pin 1,2,3,4,26 - S - Signify's it's selectable in the GUI
//   A1F26
// Analog one is fixed on pin 26

void getFeatures(DynamicJsonDocument &json)
{
  JsonArray jsfeat = json.createNestedArray("Feat");
  #ifdef HAS_WS2812
    jsfeat.add(FEAT_WS2812);
  #endif
  #ifdef HAS_3DIODE_RGB
    jsfeat.add(FEAT_3DIODE_RGB);
  #endif
  #ifdef HAS_POWERLED
    jsfeat.add(FEAT_POWERLED);
  #endif
  #ifdef HAS_BUZZER
    jsfeat.add(FEAT_BUZZER);
  #endif
  #ifdef HAS_NOTIFYLED
    jsfeat.add(FEAT_NOTIFYLED);
  #endif
  #ifdef HAS_MPU6500
    jsfeat.add(FEAT_MPU6500);
  #endif
  #ifdef HAS_QMC5883
    jsfeat.add(FEAT_QMC5883);
  #endif
  #ifdef HAS_APDS9960
    if(blesenseboard) // This sensor is detected at powerup, Nano33 vs Nano33 Sense
      jsfeat.add(FEAT_APDS9960);
  #endif
  #ifdef HAS_BMI270
    jsfeat.add(FEAT_BMI270);
  #endif
  #ifdef HAS_MPU9050
    jsfeat.add(FEAT_MPU9050);
  #endif
  #ifdef HAS_BMM150
    jsfeat.add(FEAT_BMM150);
  #endif
  #ifdef HAS_LSM9DS1
    jsfeat.add(FEAT_LSM9DS1);
  #endif
  #ifdef HAS_LSM6DS3
    jsfeat.add(FEAT_LSM6DS3);
  #endif
  #ifdef HAS_CENTERBTN
    jsfeat.add(FEAT_CENTERBTN);
  #endif
  #ifdef HAS_PPM
    // Pin number and decription should be added here
    jsfeat.add(FEAT_PPMIN);
  #endif
  #ifdef HAS_PPM
    // Pin number and decription should be added here
    jsfeat.add(FEAT_PPMOUT);
  #endif
  #ifdef HAS_PWM4
    jsfeat.add(FEAT_PWM4);
  #endif
  #ifdef HAS_USBHID
    jsfeat.add(FEAT_USBHID);
  #endif
  #ifdef HAS_UART
    jsfeat.add(FEAT_UART);
  #endif
  #ifdef HAS_UART_INV_REQ_SHORT
    jsfeat.add(FEAT_UART_INV_REQ_SHORT);
  #endif
  #ifdef HAS_BT4
    jsfeat.add(FEAT_BT4);
  #endif
  #ifdef HAS_BT5
    jsfeat.add(FEAT_BT5);
  #endif
  #ifdef HAS_BTEDR
    jsfeat.add(FEAT_BTEDR);
  #endif
  #ifdef HAS_WIFI
    jsfeat.add(FEAT_WIFI);
  #endif
  #ifdef AN0
    jsfeat.add(FEAT_ANALOG1);
  #endif
  #ifdef AN1
    jsfeat.add(FEAT_ANALOG2);
  #endif
  #ifdef AN2
    jsfeat.add(FEAT_ANALOG3);
  #endif
  #ifdef AN3
    jsfeat.add(FEAT_ANALOG4);
  #endif
  #ifdef HAS_BATTMON
    jsfeat.add(FEAT_BATTMON);
  #endif
  #if defined(HAS_LSM9DS1) || defined(HAS_QMC5883) || defined(HAS_BMM150)
    jsfeat.add(FEAT_MAGNETOMETER);
  #endif
}
