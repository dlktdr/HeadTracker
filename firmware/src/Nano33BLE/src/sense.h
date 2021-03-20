#ifndef SENSE_H
#define SENSE_H

#define GYRO_DEADBAND 0.0 // Gyro deadband, seems to help with this sensor
#define APDS_HYSTERISIS 10

// Oversample Setting
#if defined(MAHONY) || defined(MADGWICK)
    const int SENSEUPDATE = 1;
#else
    const int SENSEUPDATE = 6;
#endif

int sense_Init();
void sense_Thread();
float normalize( const float value, const float start, const float end );
void rotate(float pn[3], const float rot[3]);
void reset_fusion();

//extern uint16_t zaccelout;

#endif