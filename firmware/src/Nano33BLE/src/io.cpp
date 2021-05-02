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
#include "trackersettings.h"
#include "main.h"
#include "io.h"

volatile bool buttonpressed=false;
volatile int butpin;

void io_Init()
{
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LEDR, OUTPUT);
    pinMode(LEDG, OUTPUT);
    pinMode(LEDB, OUTPUT);
    digitalWrite(LEDR,HIGH);
    digitalWrite(LEDG,HIGH);
    digitalWrite(LEDB,HIGH);

    // Pins used to check timing
    /*pinMode(A0, OUTPUT);//    PWM 0
    pinMode(A1, OUTPUT);  //    PWM 1
    pinMode(A2, OUTPUT);  //    PWM 2
    pinMode(A3, OUTPUT); //     PWM 3 */
    pinMode(A4, OUTPUT); //
    pinMode(A5, OUTPUT); //
    pinMode(A6, INPUT); // Analog input
    pinMode(A7, INPUT); // Analog input

    // Set analog read resolution to 12bit
    analogReadResolution(ANALOG_RESOLUTION);

    butpin = trkset.buttonPin();
}

// Reset Button Pressed Flag on Read
bool wasButtonPressed() {
  if(buttonpressed) {
    buttonpressed = false;
    return true;
  }
  return false;
}

void pressButton() {
  buttonpressed = true;
}

// Any IO Related Tasks, buttons, etc.. ISR. Run at 1Khz
void io_Task()
{
  static int i =0;
  // Fast Blink to know it's running
  if(i==5) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  if(i==10) {
    digitalWrite(LED_BUILTIN, LOW);
    i=0;
  }
  i++;

  // Make sure button pin is enabled
  if(butpin < 1 || butpin > 13 )
    return;

  // Check button inputs, set flag, could make this an ISR but button for sure will be down for at least 1ms, also debounces
  if(digitalRead(butpin) == 0)
    buttonpressed = true;
}