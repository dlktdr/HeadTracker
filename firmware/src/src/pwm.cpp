/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2021 Cliff Blackburn
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* 4 Channels of PWM output, limited to 200Hz - 31Hz Update Rates
 *  31 Comes from coounter top max value = 32767us
 *   1/32768 = 30.5Hz
 *    */

#include "pmw.h"
#include "trackersettings.h"


static uint16_t pwmvals[4];
static constexpr uint16_t countertop = 5000;  // 5000uS per period, before end delay
static uint16_t usperiod = 5000;

int PWM_Init(int updateRate)
{
  if (updateRate > 200 || updateRate < 31) return -1;

  usperiod = (1 / (float)updateRate * 1000000);

  for (int i = 0; i < 4; i++) {
    pwmvals[i] = usperiod - TrackerSettings::PPM_CENTER;
  }

  NRF_PWM0->PSEL.OUT[0] =
      (4 << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
  NRF_PWM0->PSEL.OUT[1] =
      (5 << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
  NRF_PWM0->PSEL.OUT[2] =
      (30 << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);
  NRF_PWM0->PSEL.OUT[3] =
      (29 << PWM_PSEL_OUT_PIN_Pos) | (PWM_PSEL_OUT_CONNECT_Connected << PWM_PSEL_OUT_CONNECT_Pos);

  NRF_PWM0->ENABLE = (PWM_ENABLE_ENABLE_Enabled << PWM_ENABLE_ENABLE_Pos);
  NRF_PWM0->MODE = (PWM_MODE_UPDOWN_Up << PWM_MODE_UPDOWN_Pos);
  NRF_PWM0->PRESCALER =
      (PWM_PRESCALER_PRESCALER_DIV_16 << PWM_PRESCALER_PRESCALER_Pos);  // 1Mhz 1uS Resolution
  NRF_PWM0->COUNTERTOP = (usperiod << PWM_COUNTERTOP_COUNTERTOP_Pos);   // 1 msec
  NRF_PWM0->LOOP = 0;
  NRF_PWM0->DECODER = (PWM_DECODER_LOAD_Individual << PWM_DECODER_LOAD_Pos) |
                      (PWM_DECODER_MODE_RefreshCount << PWM_DECODER_MODE_Pos);
  NRF_PWM0->SEQ[0].PTR = ((uint32_t)(pwmvals) << PWM_SEQ_PTR_PTR_Pos);
  NRF_PWM0->SEQ[0].CNT = ((sizeof(pwmvals) / sizeof(uint16_t)) << PWM_SEQ_CNT_CNT_Pos);
  NRF_PWM0->SEQ[0].REFRESH = 1;
  NRF_PWM0->SEQ[0].ENDDELAY = 0;

  NRF_PWM0->TASKS_SEQSTART[0] = 1;
  return 0;
}

void setPWMValue(int ch, uint16_t value)
{
  if (ch < 0 || ch > 3) return;

  // Set PPM Channel, Limited to min/max
  pwmvals[ch] = usperiod - MAX(MIN(value, TrackerSettings::MAX_PWM), TrackerSettings::MIN_PWM);

  NRF_PWM0->TASKS_SEQSTART[0] = 1;
}