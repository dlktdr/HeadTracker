#ifndef IO_H
#define IO_H

#define ANALOG_RESOLUTION 12

bool wasButtonPressed();
void pressButton();
void io_Task();
void io_Init();
extern volatile bool buttonpressed;

#endif