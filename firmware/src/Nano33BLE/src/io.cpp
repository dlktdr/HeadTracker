#include <Arduino.h>
#include "trackersettings.h"
#include "main.h"
#include "io.h"

volatile bool buttonpressed=false;

void io_Init()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDR,HIGH);
  digitalWrite(LEDG,HIGH);
  digitalWrite(LEDB,HIGH);
}

// Reset Button Pressed Flag on Read
bool wasButtonPressed() {
  if(buttonpressed) {
    __disable_irq();
    buttonpressed = false;
    __enable_irq();
    return true;
  }
  return false;
}

void pressButton() {
  __disable_irq();
  buttonpressed = true;
  __enable_irq();
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

  // Check button inputs, set flag, could make this an ISR but button for sure will be down for at least 1ms, also debounces
  if(digitalRead(trkset.buttonPin()) == 0)
    buttonpressed = true; 
}