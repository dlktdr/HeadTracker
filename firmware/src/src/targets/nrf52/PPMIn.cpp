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

#include "PPMIn.h"

#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <zephyr.h>

#include <chrono>

#include "defines.h"
#include "io.h"
#include "log.h"
#include "trackersettings.h"

#define PPMIN_PPICH1_MSK CONCAT(CONCAT(PPI_CHENSET_CH, PPMIN_PPICH1), _Msk)
#define PPMIN_PPICH2_MSK CONCAT(CONCAT(PPI_CHENSET_CH, PPMIN_PPICH2), _Msk)
#define PPMIN_TIMER CONCAT(NRF_TIMER, PPMIN_TIMER_CH)
//#define PPMIN_GPIO_IRQNO CONCAT(CONCAT(TIMER, PPMIN_TIMER_CH), _IRQn )
#define PPMIN_GPIOTE_MASK CONCAT(CONCAT(GPIOTE_INTENSET_IN, PPMIN_GPIOTE), _Msk)

static bool framestarted = false;
static bool ppminstarted = false;
static bool ppminverted = false;
static int setPin = -1;

// Used in ISR
static uint16_t isrchannels[16];
static int isrch_count = 0;

// Used to read data at once, read with isr disabled
static uint16_t channels[16];
static int ch_count = 0;

static volatile uint64_t runtime = 0;

ISR_DIRECT_DECLARE(PPMInGPIOTE_ISR)
{
  ISR_DIRECT_HEADER();

  if (NRF_GPIOTE->EVENTS_IN[PPMIN_GPIOTE]) {
    // Clear Flag
    NRF_GPIOTE->EVENTS_IN[PPMIN_GPIOTE] = 0;

    // Read Timer Captured Value
    uint32_t time = PPMIN_TIMER->CC[PPMIN_TMRCOMP_CH];

    // Long pulse = Start.. Minimum frame sync is 3ms.. Giving a 10us leway
    if (time > 2990) {
      // Copy all data to another buffer so it can be read complete
      for (int i = 0; i < 16; i++) {
        ch_count = isrch_count;
        channels[i] = isrchannels[i];
      }
      isrch_count = 0;
      framestarted = true;

      // Used to check if a signal is here
      runtime = micros64();

      // Valid Ch Range
    } else if (time > 900 && time < 2100 && framestarted == true && isrch_count < 16) {
      isrchannels[isrch_count] = time;
      isrch_count++;

      // Fault, Reset
    } else {
      isrch_count = 0;
      framestarted = false;
    }
  }

  ISR_DIRECT_FOOTER(1);
  return 0;
}

// Set pin to -1 to disable
void PpmIn_setPin(int pinNum)
{
  // Same pin, just quit
  if (pinNum == setPin) return;

#if defined(PCB_NANO33BLE) // Nano33, can pick D2-D12
  int pin = D_TO_PIN(pinNum);
  int port = D_TO_PORT(pinNum);
  int setPin_pin = D_TO_PIN(setPin);
  int setPin_port = D_TO_PORT(setPin);
#else
  int pin = PIN_TO_NRFPIN(PIN_NAME_TO_NUM(IO_PPMIN));
  int port = PIN_TO_NRFPORT(PIN_NAME_TO_NUM(IO_PPMIN));
  int setPin_pin = pin;
  int setPin_port = port;
#endif

  // Stop Interrupts
  uint32_t key = irq_lock();

  if (pinNum < 0 && ppminstarted) {  // Disable

    // Stop Interrupt
    NRF_GPIOTE->INTENCLR = PPMIN_GPIOTE_MASK;

    // Clear interrupt flag
    NRF_GPIOTE->EVENTS_IN[PPMIN_GPIOTE] = 0;
    NRF_GPIOTE->CONFIG[PPMIN_GPIOTE] = 0;  // Disable Config

    // Disable PPIs
    NRF_PPI->CHENCLR = PPMIN_PPICH1_MSK | PPMIN_PPICH2_MSK;

    irq_disable(GPIOTE_IRQn);

    // Set current pin back floating
    if (setPin_port == 0)
      NRF_P0->PIN_CNF[setPin_pin] =
          (NRF_P0->PIN_CNF[setPin_pin] & ~GPIO_PIN_CNF_PULL_Msk) |
          GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos;
    else if (setPin_port == 1)
      NRF_P1->PIN_CNF[setPin_pin] =
          (NRF_P1->PIN_CNF[setPin_pin] & ~GPIO_PIN_CNF_PULL_Msk) |
          GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos;

    // Set pin num and started flag
    setPin = pinNum;
    ppminstarted = false;

  } else {
    setPin = pinNum;

    // Disable Interrupt, Clear event
    NRF_GPIOTE->INTENCLR = PPMIN_GPIOTE_MASK;
    NRF_GPIOTE->EVENTS_IN[PPMIN_GPIOTE] = 0;

    // Set pin pull up enabled
    if (port == 0)
      NRF_P0->PIN_CNF[pin] = (NRF_P0->PIN_CNF[pin] & ~GPIO_PIN_CNF_PULL_Msk) |
                             GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos;
    else if (port == 1)
      NRF_P1->PIN_CNF[pin] = (NRF_P1->PIN_CNF[pin] & ~GPIO_PIN_CNF_PULL_Msk) |
                             GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos;

    if (!ppminverted) {
      NRF_GPIOTE->CONFIG[PPMIN_GPIOTE] =
          (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
          (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos) |
          (pin << GPIOTE_CONFIG_PSEL_Pos) | (port << GPIOTE_CONFIG_PORT_Pos);
    } else {
      NRF_GPIOTE->CONFIG[PPMIN_GPIOTE] =
          (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
          (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos) |
          (pin << GPIOTE_CONFIG_PSEL_Pos) | (port << GPIOTE_CONFIG_PORT_Pos);
    }

    // First time starting?
    if (!ppminstarted) {
      // Start Timers, they can stay running all the time.
      PPMIN_TIMER->PRESCALER = 4;  // 16Mhz/2^4 = 1Mhz = 1us Resolution, 1.048s Max@32bit
      PPMIN_TIMER->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
      PPMIN_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;

      // Start timer
      PPMIN_TIMER->TASKS_START = 1;

      // On Transition, Capture Timer
      NRF_PPI->CH[PPMIN_PPICH1].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[PPMIN_GPIOTE];
      NRF_PPI->CH[PPMIN_PPICH1].TEP = (uint32_t)&PPMIN_TIMER->TASKS_CAPTURE[PPMIN_TMRCOMP_CH];
      // NRF_PPI->FORK[8].TEP = (uint32_t)&NRF_TIMER4->TASKS_CLEAR;

      // On Transition, Clear Timer
      NRF_PPI->CH[PPMIN_PPICH2].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[PPMIN_GPIOTE];
      NRF_PPI->CH[PPMIN_PPICH2].TEP = (uint32_t)&PPMIN_TIMER->TASKS_CLEAR;

      // Enable PPIs
      NRF_PPI->CHENSET = PPMIN_PPICH1_MSK | PPMIN_PPICH2_MSK;

      // Set our handler
      IRQ_DIRECT_CONNECT(GPIOTE_IRQn, 0, PPMInGPIOTE_ISR, IRQ_ZERO_LATENCY);
      irq_enable(GPIOTE_IRQn);

      ppminstarted = true;
    }

    // Clear flags and enable interrupt
    NRF_GPIOTE->EVENTS_IN[PPMIN_GPIOTE] = 0;
    NRF_GPIOTE->INTENSET |= PPMIN_GPIOTE_MASK;
  }

  irq_unlock(key);
}

void PpmIn_setInverted(bool inv)
{
  if (!ppminstarted) return;

  if (ppminverted != inv) {
    int op = setPin;
    PpmIn_setPin(-1);
    ppminverted = inv;
    PpmIn_setPin(op);
  }
}

static int cyclescount = 0;  // Count this many cycles before showing data
#define cyclesbeforeppm 20

void PpmIn_execute()
{
  static bool sentconn = false;

  if (micros64() - runtime > 60000) {
    if (sentconn == false) {
      LOGW("PPM Input Data Lost");
      sentconn = true;
      ch_count = 0;
    }
    cyclescount = 0;  // No Data
  } else {
    if (cyclescount < cyclesbeforeppm)
      cyclescount++;
    else {
      if (sentconn == true && ch_count >= 4 && ch_count <= 16) {
        LOGI("PPM Input Data Received");
        sentconn = false;
      }
    }
  }

  // Check if Inverted state has changed
  if(ppminverted != trkset.getPpmInInvert())
  {
    PpmIn_setInverted(trkset.getPpmInInvert());
  }
}

// Returns number of channels read
int PpmIn_getChannels(uint16_t *ch)
{
  if (!ppminstarted || cyclescount < cyclesbeforeppm) return 0;

  __disable_irq();
  for (int i = 0; i < ch_count; i++) {
    ch[i] = channels[i];
  }
  int rval = ch_count;
  __enable_irq();

  return rval;
}