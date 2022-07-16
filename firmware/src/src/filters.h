#pragma once

void filter_lowPass(float input, float *output, float beta) { *output += beta * (input - *output); }

void filter_expAverage(float *variable, float beta, float *buffer)
{
  *variable = beta * *variable + (1.0 - beta) * *buffer;
  *buffer = *variable;
}
