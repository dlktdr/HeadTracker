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
#include <string.h>
#include <nrfx.h>
#include "uarte_sbus.h"

#define BAUD100000                   0x0198EF80
#define CONF8E2                      0x0000001E
#define SBUS_FRAME_LEN 25

static constexpr uint8_t HEADER_ = 0x0F;
static constexpr uint8_t FOOTER_ = 0x00;
static constexpr uint8_t FOOTER2_ = 0x04;
static constexpr uint8_t LEN_ = 25;
static constexpr uint8_t CH17_ = 0x01;
static constexpr uint8_t CH18_ = 0x02;
static constexpr uint8_t LOST_FRAME_ = 0x04;
static constexpr uint8_t FAILSAFE_ = 0x08;
static bool failsafe_ = false, lost_frame_ = false, ch17_ = false, ch18_ = false;

uint8_t sbusDMATx[SBUS_FRAME_LEN]; // DMA Access Buffer
uint8_t localTXBuffer[SBUS_FRAME_LEN]; // Local Buffer

volatile bool isTransmitting=false;
volatile bool isSBUSInit=false;

void SBUS_Thread()
{
    while(1) {
        if(isSBUSInit)
            SBUS_TX_Start();
        k_msleep(SBUS_PERIOD);
    }
}

ISR_DIRECT_DECLARE(SBUS_TX_Complete_Interrupt)
{
    ISR_DIRECT_HEADER();
    // Clear Event
    NRF_UARTE1->EVENTS_ENDTX = 0;
    isTransmitting = false;

    // Disable PPI 11
    NRF_PPI->CHEN |= (PPI_CHEN_CH11_Enabled << PPI_CHEN_CH11_Pos);
    ISR_DIRECT_FOOTER(1);
    return 0;
}

void SBUS_TX_Start()
{
    // Already Transmitting, called too soon.
    if(isTransmitting)
        return;

    // Copy Data from local buffer
    memcpy(sbusDMATx,localTXBuffer,SBUS_FRAME_LEN);

    // Initiate the transfer
    isTransmitting = true;
    NRF_UARTE1->TASKS_STARTTX = 1;
}

void SBUS_Init(int txPin, int invTxPin)
{
    isSBUSInit = false;

    // Disable UART1 + Interrupt & IRQ controller
    NRF_UARTE1->ENABLE = UARTE_ENABLE_ENABLE_Disabled << UARTE_ENABLE_ENABLE_Pos;
    NRF_UARTE1->EVENTS_ENDTX = 0;
    irq_disable(UARTE1_IRQn);
    NRF_UARTE1->INTENCLR = UARTE_INTENSET_ENDTX_Msk;

    // Baud 100000, 8E2
    // 25 Byte Data Send Length
    NRF_UARTE1->BAUDRATE = BAUD100000;
    NRF_UARTE1->CONFIG = CONF8E2;

    // DMA Access space for SBUS output
    NRF_UARTE1->TXD.PTR = (uint32_t)sbusDMATx;
    NRF_UARTE1->TXD.MAXCNT =SBUS_FRAME_LEN;

    int dpintopin[]  = {3,10,11,12,15,13,14,23,21,27,2,1,8,13};
    int dpintoport[] = {1,1,1 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,1,1,1,0 };

    int tpin = dpintopin[txPin];
    int tport = dpintoport[txPin];
    int invtxpin = dpintopin[invTxPin];
    int invtxport = dpintoport[invTxPin];

    // Only Enable TX Pin
    NRF_UARTE1->PSEL.TXD = (tpin << UARTE_PSEL_TXD_PIN_Pos) | (tport << UARTE_PSEL_TXD_PORT_Pos);
    NRF_UARTE1->PSEL.RXD = UARTE_PSEL_RXD_CONNECT_Disconnected << UARTE_PSEL_RXD_CONNECT_Pos;
    NRF_UARTE1->PSEL.CTS = UARTE_PSEL_CTS_CONNECT_Disconnected << UARTE_PSEL_CTS_CONNECT_Pos;
    NRF_UARTE1->PSEL.RTS = UARTE_PSEL_RTS_CONNECT_Disconnected << UARTE_PSEL_RTS_CONNECT_Pos;

    // Below uses two GPIOTE's to read the TX pin and cause it to be inverted on the RX pin

    // Setup as an input, when TX pin toggles state state causes the event to trigger and through
    // PPI toggle the output pin on next GPIOTE

    NRF_GPIOTE->CONFIG[4] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (tpin <<  GPIOTE_CONFIG_PSEL_Pos) |
            (tport << GPIOTE_CONFIG_PORT_Pos);

    NRF_GPIOTE->CONFIG[5] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (invtxpin <<  GPIOTE_CONFIG_PSEL_Pos) |
            (invtxport << GPIOTE_CONFIG_PORT_Pos);// |
            //(1 << GPIOTE_CONFIG_OUTINIT_Pos);            // Initial value of pin low

    // On Compare equals Value, Toggle IO Pin
    NRF_PPI->CH[11].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[4];
    NRF_PPI->CH[11].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[5];

    // Enable PPI 11
    NRF_PPI->CHEN |= (PPI_CHEN_CH11_Enabled << PPI_CHEN_CH11_Pos);

    // Enable the interrupt vector in IRQ Controller
    //NVIC_SetVector(UARTE1_IRQn,(uint32_t)&SBUS_TX_Complete_Interrupt);
    IRQ_DIRECT_CONNECT(UARTE1_IRQn,0,SBUS_TX_Complete_Interrupt,0);
    irq_enable(UARTE1_IRQn);

    // Enable interupt in peripheral
    NRF_UARTE1->INTENSET = UARTE_INTENSET_ENDTX_Msk;

    // Enable UART1
    NRF_UARTE1->ENABLE = UARTE_ENABLE_ENABLE_Enabled << UARTE_ENABLE_ENABLE_Pos;
    isSBUSInit = true;
}


// Build Channel Data

/*  FROM -----
* Brian R Taylor
* brian.taylor@bolderflight.com
*
* Copyright (c) 2021 Bolder Flight Systems Inc
*/

void SBUS_TX_BuildData(uint16_t ch_[16])
{
    uint8_t *buf_ = localTXBuffer;
    buf_[0] = HEADER_;
    buf_[1] =  static_cast<uint8_t>((ch_[0]  & 0x07FF));
    buf_[2] =  static_cast<uint8_t>((ch_[0]  & 0x07FF) >> 8  |
                (ch_[1]  & 0x07FF) << 3);
    buf_[3] =  static_cast<uint8_t>((ch_[1]  & 0x07FF) >> 5  |
                (ch_[2]  & 0x07FF) << 6);
    buf_[4] =  static_cast<uint8_t>((ch_[2]  & 0x07FF) >> 2);
    buf_[5] =  static_cast<uint8_t>((ch_[2]  & 0x07FF) >> 10 |
                (ch_[3]  & 0x07FF) << 1);
    buf_[6] =  static_cast<uint8_t>((ch_[3]  & 0x07FF) >> 7  |
                (ch_[4]  & 0x07FF) << 4);
    buf_[7] =  static_cast<uint8_t>((ch_[4]  & 0x07FF) >> 4  |
                (ch_[5]  & 0x07FF) << 7);
    buf_[8] =  static_cast<uint8_t>((ch_[5]  & 0x07FF) >> 1);
    buf_[9] =  static_cast<uint8_t>((ch_[5]  & 0x07FF) >> 9  |
                (ch_[6]  & 0x07FF) << 2);
    buf_[10] = static_cast<uint8_t>((ch_[6]  & 0x07FF) >> 6  |
                (ch_[7]  & 0x07FF) << 5);
    buf_[11] = static_cast<uint8_t>((ch_[7]  & 0x07FF) >> 3);
    buf_[12] = static_cast<uint8_t>((ch_[8]  & 0x07FF));
    buf_[13] = static_cast<uint8_t>((ch_[8]  & 0x07FF) >> 8  |
                (ch_[9]  & 0x07FF) << 3);
    buf_[14] = static_cast<uint8_t>((ch_[9]  & 0x07FF) >> 5  |
                (ch_[10] & 0x07FF) << 6);
    buf_[15] = static_cast<uint8_t>((ch_[10] & 0x07FF) >> 2);
    buf_[16] = static_cast<uint8_t>((ch_[10] & 0x07FF) >> 10 |
                (ch_[11] & 0x07FF) << 1);
    buf_[17] = static_cast<uint8_t>((ch_[11] & 0x07FF) >> 7  |
                (ch_[12] & 0x07FF) << 4);
    buf_[18] = static_cast<uint8_t>((ch_[12] & 0x07FF) >> 4  |
                (ch_[13] & 0x07FF) << 7);
    buf_[19] = static_cast<uint8_t>((ch_[13] & 0x07FF) >> 1);
    buf_[20] = static_cast<uint8_t>((ch_[13] & 0x07FF) >> 9  |
                (ch_[14] & 0x07FF) << 2);
    buf_[21] = static_cast<uint8_t>((ch_[14] & 0x07FF) >> 6  |
                (ch_[15] & 0x07FF) << 5);
    buf_[22] = static_cast<uint8_t>((ch_[15] & 0x07FF) >> 3);
    buf_[23] = 0x00 | (ch17_ * CH17_) | (ch18_ * CH18_) |
                (failsafe_ * FAILSAFE_) | (lost_frame_ * LOST_FRAME_);
    buf_[24] = FOOTER_;
}

