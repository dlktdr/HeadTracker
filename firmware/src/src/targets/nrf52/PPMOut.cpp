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

#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <sys/util.h>
#include <zephyr.h>

#include "defines.h"
#include "io.h"
#include "serial.h"
#include "trackersettings.h"


#define PPMOUT_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, PPMOUT_PPICH), _Msk)
#define PPMOUT_TIMER CONCAT(NRF_TIMER, PPMOUT_TIMER_CH)
#define PPMOUT_TIMER_IRQNO CONCAT(CONCAT(TIMER, PPMOUT_TIMER_CH), _IRQn)
#define PPMOUT_TMRCOMP_CH_MSK CONCAT(CONCAT(TIMER_INTENSET_COMPARE, PPMOUT_TMRCOMP_CH), _Msk)

volatile bool interrupt = false;

static volatile bool ppmoutstarted = false;
static volatile bool ppmoutinverted = false;
static int setPin = -1;

// Used in ISR

// Used to read data at once, read with isr disabled
static uint16_t ch_values[16];
static int ch_count;

static uint16_t framesync = TrackerSettings::PPM_MIN_FRAMESYNC;  // Minimum Frame Sync Pulse
static int32_t framelength;     // Ideal frame length
static uint16_t sync;            // Sync Pulse Length

// Local data - Only build with interrupts disabled
static uint32_t chsteps[35]{framesync, sync};

// ISR Values - Values from chsteps are copied here on step 0.
// prevent an update from happening mid stream.
static uint32_t isrchsteps[35]{framesync, sync};
static uint16_t chstepcnt = 1;
static uint16_t curstep = 0;
volatile bool buildingdata = false;

/* Builds an array with all the transition times
 */
void buildChannels()
{
  buildingdata = true;  // Prevent a read happing while this is building

  // Set user defined channel count, frame len, sync pulse
  ch_count = trkset.getPpmChCnt();
  sync = trkset.getPpmSync();
  framelength = trkset.getPpmFrame();

  int ch = 0;
  int i;
  uint32_t curtime = framesync;
  chsteps[0] = curtime;
  for (i = 1; i < ch_count * 2 + 1; i += 2) {
    curtime += sync;
    chsteps[i] = curtime;
    curtime += (ch_values[ch++] - sync);
    chsteps[i + 1] = curtime;
  }
  // Add Final Sync
  curtime += sync;
  chsteps[i++] = curtime;
  chstepcnt = i;
  // Now we know how long the train is. Try to make the entire frame == framelength
  // If possible it will add this to the frame sync pulse
  int ft = framelength - curtime;
  if (ft < 0)  // Not possible, no time left
    ft = 0;
  chsteps[i] = ft;  // Store at end of sequence
  buildingdata = false;
}

void resetChannels()
{
  // Set all channels to center
  for (int i = 0; i < 16; i++) ch_values[i] = 1500;
}

ISR_DIRECT_DECLARE(PPMTimerISR)
{
  ISR_DIRECT_HEADER();
  if (PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] == 1) {
    // Clear event
    PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] = 0;

    // Reset, don't get stuck in the wrong signal level
    if (curstep == 0) {
      if (ppmoutinverted)
        NRF_GPIOTE->TASKS_SET[PPMOUT_GPIOTE] = 1;
      else
        NRF_GPIOTE->TASKS_CLR[PPMOUT_GPIOTE] = 1;
    }

    curstep++;
    // Loop
    if (curstep >= chstepcnt) {
      if (!buildingdata) memcpy(isrchsteps, chsteps, sizeof(uint32_t) * 35);
      PPMOUT_TIMER->TASKS_CLEAR = 1;
      curstep = 0;
    }

    // Setup next capture event value
    PPMOUT_TIMER->CC[PPMOUT_TMRCOMP_CH] =
        isrchsteps[curstep] +
        isrchsteps[chstepcnt];  // Offset by the extra time required to make frame length right
  }
  ISR_DIRECT_FOOTER(1);
  return 0;
}

// Set pin to -1 to disable

void PpmOut_setPin(int pinNum)
{
  // Same pin, just quit
  if (pinNum == setPin) return;

#if defined(PCB_NANO33BLE) // Nano33, can pick D2-D12
  int pin = D_TO_PIN(pinNum);
  int port = D_TO_PORT(pinNum);
  int setPin_pin = D_TO_PIN(setPin);
  int setPin_port = D_TO_PORT(setPin);
#else
  int pin = PIN_TO_NRFPIN(PIN_NAME_TO_NUM(IO_PPMOUT));
  int port = PIN_TO_NRFPORT(PIN_NAME_TO_NUM(IO_PPMOUT));
  int setPin_pin = pin;
  int setPin_port = port;
#endif

  if (!ppmoutstarted) {
    resetChannels();
    buildChannels();
  }

  // Start by disabling

  // Stop Interrupts
  uint32_t key = irq_lock();

  // Disable Timer 3 Interrupt
  irq_disable(PPMOUT_TIMER_IRQNO);

  // Stop Timer
  PPMOUT_TIMER->TASKS_STOP = 1;

  // Stop Interrupt on peripherial
  PPMOUT_TIMER->INTENCLR = PPMOUT_TMRCOMP_CH_MSK;

  // Clear interrupt flag
  PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] = 0;
  NRF_GPIOTE->CONFIG[PPMOUT_GPIOTE] = 0;  // Disable Config

  // Disable PPI
  NRF_PPI->CHENCLR = PPMOUT_PPICH_MSK;

  // Set current pin back to low drive  , if enabled
  if (setPin > 0) {
    if (setPin_port == 0)
      NRF_P0->PIN_CNF[setPin_pin] =
          (NRF_P0->PIN_CNF[setPin_pin] & ~GPIO_PIN_CNF_DRIVE_Msk) |
          GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos;
    else if (setPin_port == 1)
      NRF_P1->PIN_CNF[setPin_pin] =
          (NRF_P1->PIN_CNF[setPin_pin] & ~GPIO_PIN_CNF_DRIVE_Msk) |
          GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos;
  }

  // If we want to enable it....
  if (pinNum > 0) {
    // Setup GPOITE[7] to toggle output on every timer capture
    NRF_GPIOTE->CONFIG[PPMOUT_GPIOTE] =
        (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
        (pin << GPIOTE_CONFIG_PSEL_Pos) | (port << GPIOTE_CONFIG_PORT_Pos);

    // High Drive PPM Output Pin
    if (port == 0)
      NRF_P0->PIN_CNF[pin] = (NRF_P0->PIN_CNF[pin] & ~GPIO_PIN_CNF_DRIVE_Msk) |
                             GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos;
    else if (port == 1)
      NRF_P1->PIN_CNF[pin] = (NRF_P1->PIN_CNF[pin] & ~GPIO_PIN_CNF_DRIVE_Msk) |
                             GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos;

    // Start Timers, they can stay running all the time.
    PPMOUT_TIMER->PRESCALER = 4;  // 16Mhz/2^(4) = 1Mhz = 1us Resolution, 1.048s Max@32bit
    PPMOUT_TIMER->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
    PPMOUT_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;

    // On Compare equals Value, Toggle IO Pin
    NRF_PPI->CH[PPMOUT_PPICH].EEP = (uint32_t)&PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH];
    NRF_PPI->CH[PPMOUT_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[PPMOUT_GPIOTE];

    // Enable PPI
    NRF_PPI->CHENSET = PPMOUT_PPICH_MSK;

    // Start
    IRQ_DIRECT_CONNECT(PPMOUT_TIMER_IRQNO, 0, PPMTimerISR, IRQ_ZERO_LATENCY);

    // Start timer
    memcpy(isrchsteps, chsteps, sizeof(uint32_t) * 32);
    PPMOUT_TIMER->CC[PPMOUT_TMRCOMP_CH] = framesync;
    curstep = 0;
    PPMOUT_TIMER->TASKS_CLEAR = 1;
    PPMOUT_TIMER->TASKS_START = 1;

    irq_enable(PPMOUT_TIMER_IRQNO);

    ppmoutstarted = true;

    // Enable timer interrupt
    PPMOUT_TIMER->EVENTS_COMPARE[PPMOUT_TMRCOMP_CH] = 0;
    PPMOUT_TIMER->INTENSET = PPMOUT_TMRCOMP_CH_MSK;
  }

  setPin = pinNum;
  irq_unlock(key);
}

void PpmOut_execute()
{
  if(trkset.getPpmOutInvert() != ppmoutinverted) {
    ppmoutinverted = trkset.getPpmOutInvert();
  }
}

void PpmOut_setChannel(int chan, uint16_t val)
{
  if (chan >= 0 && chan <= ch_count && val >= TrackerSettings::MIN_PWM &&
      val <= TrackerSettings::MAX_PWM) {
    ch_values[chan] = val;
  }
  buildChannels();
}

int PpmOut_getChnCount() { return ch_count; }
