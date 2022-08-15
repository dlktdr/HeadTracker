#pragma once

void DcmAHRSInitialize(float U0[3], float U1[3], float U2[3]);
void DcmAhrsResetCenter();
void DcmCalculate(float U0[3], float U1[3], float U2[3], float deltat);
float DcmGetTilt();
float DcmGetRoll();
float DcmGetPan();