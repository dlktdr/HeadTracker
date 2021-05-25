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


#include <zephyr.h>
#include <nrfx_ppi.h>
#include <nrfx_gpiote.h>
#include <chrono>
#include "serial.h"
#include "defines.h"
#include "PPMIn.h"

static bool framestarted=false;
static bool ppminstarted=false;
static bool ppminverted=false;
static int setPin=-1;

// Used in ISR
static uint16_t isrchannels[16];
static int isrch_count=0;

// Used to read data at once, read with isr disabled
static uint16_t channels[16];
static int ch_count=0;

static volatile uint32_t runtime = 0;

ISR_DIRECT_DECLARE(PPMInGPIOTE_ISR)
{
    ISR_DIRECT_HEADER();

    if(NRF_GPIOTE->EVENTS_IN[6]) {
        // Clear Flag
        NRF_GPIOTE->EVENTS_IN[6] = 0;

        // Read Timer Captured Value
        uint32_t time = NRF_TIMER4->CC[0];

        // Long pulse = Start.. Minimum frame sync is 4ms.. Giving a 10us leway
        if(time > 3990) {
            // Copy all data to another buffer so it can be read complete
            for(int i=0;i<16;i++) {
                ch_count = isrch_count;
                channels[i] = isrchannels[i];
            }
            isrch_count = 0;
            framestarted = true;

            // Used to check if a signal is here
            runtime = micros();


        // Valid Ch Range
        } else if(time > 900 && time < 2100 &&
                  framestarted == true &&
                  isrch_count < 16) {
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
    if(pinNum == setPin)
        return;



    // THIS MUST BE DEFINED SOMEWHERE... CAN'T FIND IT!
    int dpintopin[]  = {0,0,11,12,15,13,14,23,21,27,2,1,8,13};
    int dpintoport[] = {0,0,1 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,1,1,1,0 };

    int pin = dpintopin[pinNum];
    int port = dpintoport[pinNum];

    // Stop Interrupts
    uint32_t key = irq_lock();


    if(pinNum < 0 && ppminstarted) { // Disable

        // Stop Interrupt
        NRF_GPIOTE->INTENCLR = GPIOTE_INTENSET_IN6_Msk;

        // Clear interrupt flag
        NRF_GPIOTE->EVENTS_IN[6] = 0;
        NRF_GPIOTE->CONFIG[6] = 0; // Disable Config

        irq_disable(GPIOTE_IRQn);

        // Set pin num and started flag
        setPin = pinNum;
        ppminstarted = false;

    } else {
        setPin = pinNum;

        // Disable Interrupt, Clear event
        NRF_GPIOTE->INTENCLR = GPIOTE_INTENSET_IN6_Msk;
        NRF_GPIOTE->EVENTS_IN[6] = 0;

        if(!ppminverted) {
            NRF_GPIOTE->CONFIG[6] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
                            (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos) |
                            (pin <<  GPIOTE_CONFIG_PSEL_Pos) |
                            (port << GPIOTE_CONFIG_PORT_Pos);
        } else {
            NRF_GPIOTE->CONFIG[6] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
                            (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos) |
                            (pin <<  GPIOTE_CONFIG_PSEL_Pos) |
                            (port << GPIOTE_CONFIG_PORT_Pos);
        }

        // First time starting?
        if(!ppminstarted) {
            // Start Timers, they can stay running all the time.
            NRF_TIMER4->PRESCALER = 4; // 16Mhz/2^4 = 1Mhz = 1us Resolution, 1.048s Max@32bit
            NRF_TIMER4->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
            NRF_TIMER4->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;

            // Start timer
            NRF_TIMER4->TASKS_START = 1;

            // On Transition, Capture Timer 4
            NRF_PPI->CH[8].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[6];
            NRF_PPI->CH[8].TEP = (uint32_t)&NRF_TIMER4->TASKS_CAPTURE[0];
            //NRF_PPI->FORK[8].TEP = (uint32_t)&NRF_TIMER4->TASKS_CLEAR;

            // On Transition, Clear Timer 4
            NRF_PPI->CH[9].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[6];
            NRF_PPI->CH[9].TEP = (uint32_t)&NRF_TIMER4->TASKS_CLEAR;

            // Enable PPI 8+9
            NRF_PPI->CHEN |= (PPI_CHEN_CH8_Enabled << PPI_CHEN_CH8_Pos);
            NRF_PPI->CHEN |= (PPI_CHEN_CH9_Enabled << PPI_CHEN_CH9_Pos);

            // Set our handler
            IRQ_DIRECT_CONNECT(GPIOTE_IRQn,0,PPMInGPIOTE_ISR,IRQ_ZERO_LATENCY);

            irq_enable(GPIOTE_IRQn);

            ppminstarted = true;
        }

        // Clear flags and enable interrupt
        NRF_GPIOTE->EVENTS_IN[6] = 0;
        NRF_GPIOTE->INTENSET |= GPIOTE_INTENSET_IN6_Set << GPIOTE_INTENSET_IN6_Pos;
    }

    irq_unlock(key);
}

void PpmIn_setInverted(bool inv)
{
    if(!ppminstarted)
        return;

    if(ppminverted != inv) {
        int op = setPin;
        PpmIn_setPin(-1);
        ppminverted = inv;
        PpmIn_setPin(op);
    }
}



void PpmIn_execute()
{
    static bool sentconn=false;
    uint32_t micros = micros() - runtime;
    if(micros > 60000) {
        if(sentconn == false) {
            serialWriteln("HT: PPM Input Data Lost");
            sentconn = true;
            ch_count = 0;
        }
    } else {
        if(sentconn == true && ch_count >= 4 && ch_count <= 16) {
            serialWriteln("HT: PPM Input Data Received");
            sentconn = false;
        }
    }
}

// Returns number of channels read
int PpmIn_getChannels(uint16_t *ch)
{
    if(!ppminstarted)
        return 0;

    __disable_irq();
    for(int i=0; i < ch_count;i++) {
        ch[i] = channels[i];
    }
    int rval = ch_count;
    __enable_irq();

    return rval;
}