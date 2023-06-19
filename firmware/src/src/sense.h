#pragma once

int sense_Init();
void sensor_Thread();
void calculate_Thread();
float normalize(const float value, const float start, const float end);
void rotate(float pn[3], const float rot[3]);
void reset_fusion();
void buildAuxData();

extern bool blesenseboard;
