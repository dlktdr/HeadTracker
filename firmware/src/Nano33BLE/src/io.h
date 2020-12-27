#ifndef IO_H
#define IO_H

bool wasButtonPressed();
void pressButton();
void io_Task();
extern volatile bool buttonpressed;

#endif