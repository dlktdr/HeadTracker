#pragma once

#include <zephyr.h>



extern struct k_mutex sensor_mutex;

int sense_Init();
void sensor_Thread();
void main_Thread();
void onesecThread();
float normalize(const float value, const float start, const float end);
void rotate(float pn[3], const float rot[3]);
void reset_fusion();
void buildAuxData();
