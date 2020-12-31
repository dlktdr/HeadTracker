#ifndef SENSE_H
#define SENSE_H

// Ideal period of filter, not super accurate due low precision on 
// thread sleep. Can't seem to get faster than this
const int SLEEPTIME = 7000;

// Runs Filter 5x more than update sensors 
// 
const int SENSEUPDATE = 6; 

int sense_Init();
void sense_Thread();
float normalize( const float value, const float start, const float end );

#endif