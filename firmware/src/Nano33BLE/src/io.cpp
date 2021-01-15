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
  pinMode(A0, OUTPUT); // Sensor thread
  pinMode(A1, OUTPUT); // 
  pinMode(A2, OUTPUT); // 
  pinMode(A3, OUTPUT); // 
  pinMode(A4, OUTPUT); // 
  pinMode(A5, OUTPUT); // 
  pinMode(A6, OUTPUT); // 
  pinMode(A7, OUTPUT); // 

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
  if(i==100) {
    digitalWrite(LED_BUILTIN, HIGH);
  } 
  if(i==200) {
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