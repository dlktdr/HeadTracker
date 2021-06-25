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
#include "defines.h"
#include "io.h"
#include "trackersettings.h"
#include <nrfx.h>
#include <nrfx_uarte.h>

#define BAUD100000       0x0198EF80
#define CONF8E2          0x0000001E
#define SBUS_FRAME_LEN   25

#define SBUSOUT_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SBUSOUT_PPICH), _Msk )
#define SBUSOUT_UARTE CONCAT(NRF_UARTE, SBUSOUT_UARTE_CH)
#define SBUSOUT_UARTE_IRQ CONCAT(CONCAT(UARTE, SBUSOUT_UARTE_CH),_IRQn)

static constexpr uint8_t HEADER_ = 0x0F;
static constexpr uint8_t FOOTER_ = 0x00;
static constexpr uint8_t FOOTER2_ = 0x04;
static constexpr uint8_t LEN_ = 25;
static constexpr uint8_t CH17_ = 0x01;
static constexpr uint8_t CH18_ = 0x02;
static constexpr uint8_t LOST_FRAME_ = 0x04;
static constexpr uint8_t FAILSAFE_ = 0x08;
static bool failsafe_ = false, lost_frame_ = false, ch17_ = false, ch18_ = false;

uint8_t sbusDMATx[SBUS_FRAME_LEN]; // DMA Access Buffer Write
uint8_t sbusDMARx[SBUS_FRAME_LEN]; // DMA Access Buffer Read
uint8_t localTXBuffer[SBUS_FRAME_LEN]; // Local Buffer

volatile bool isTransmitting=false;
volatile bool isSBUSInit=false;
volatile bool sbusoutinv=false;

void SBUS_Thread()
{
    while(1) {
        if(isSBUSInit) {
            sbusoutinv = !trkset.invertedSBUSOut();
            SBUS_TX_Start();
        }
        k_msleep(SBUS_PERIOD);
    }
}

void SBUS_TX_Complete_Interrupt(void *arg)
{

    //ISR_DIRECT_HEADER();
    // Clear Event
    SBUSOUT_UARTE->EVENTS_ENDTX = 0;
    isTransmitting = false;

    // Below is done to be sure every cycle invert pin is cycled
    // UARTE should be done transmitting here and pin should be high

    // Disable PPI 11
    NRF_PPI->CHENCLR = SBUSOUT_PPICH_MSK;

    // Disable GPIOTE2 - Output (Cause pin to go low)
    NRF_GPIOTE->CONFIG[SBUS_GPIOTE2] = 0;

    // Enable GPIOTE2 - Output

    uint32_t confreg =  (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (3 <<  GPIOTE_CONFIG_PSEL_Pos) |
            (1 << GPIOTE_CONFIG_PORT_Pos);
    if(!sbusoutinv)
        confreg |= GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos;

    NRF_GPIOTE->CONFIG[SBUS_GPIOTE2] = confreg;       // Initial value of pin low

    // Enable PPI 11
    NRF_PPI->CHENSET = SBUSOUT_PPICH_MSK;

  //  ISR_DIRECT_FOOTER(1);
//    return 1;
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
    SBUSOUT_UARTE->TASKS_STARTTX = 1;
}

void m_uart_context_callback()
{

}

void SBUS_Init()
{
    // SBUS Output Setup
    //   Sets the UARTE pin to unused P1.04
    //   GPIOTE is set to use the TX Pin P1.03
    //   PPI Inverts or Copys P1.04 to P1.03 Depending on the SBUS Out Invert bool in TrackerSettings

    isSBUSInit = false;

    // Bootloader leaves the UART connected.. disconnect it first.
    NRF_UART0->ENABLE = 0;
    NRF_UART0->CONFIG = 0;
    NRF_UART0->PSEL.TXD = UART_PSEL_TXD_CONNECT_Disconnected << UART_PSEL_TXD_CONNECT_Pos;
    NRF_UART0->PSEL.RXD = UART_PSEL_RXD_CONNECT_Disconnected << UART_PSEL_RXD_CONNECT_Pos;
    NRF_UART0->PSEL.CTS = UART_PSEL_CTS_CONNECT_Disconnected << UART_PSEL_CTS_CONNECT_Pos;
    NRF_UART0->PSEL.RTS = UART_PSEL_RTS_CONNECT_Disconnected << UART_PSEL_RTS_CONNECT_Pos;
    NRF_UART0->INTENCLR = 0xFFFF;

    irq_disable(SBUSOUT_UARTE_IRQ);

    // Disable UART1 + Interrupt & IRQ controller
    SBUSOUT_UARTE->EVENTS_ENDTX = 0;
    SBUSOUT_UARTE->ENABLE = UARTE_ENABLE_ENABLE_Disabled << UARTE_ENABLE_ENABLE_Pos;

    SBUSOUT_UARTE->INTENCLR = UARTE_INTENSET_ENDTX_Msk;

    // Baud 100000, 8E2
    // 25 Byte Data Send Length
    SBUSOUT_UARTE->BAUDRATE = BAUD100000;
    SBUSOUT_UARTE->CONFIG = CONF8E2;

    // DMA Access space for SBUS output
    SBUSOUT_UARTE->TXD.PTR = (uint32_t)sbusDMATx;
    SBUSOUT_UARTE->TXD.MAXCNT =SBUS_FRAME_LEN;

    // Only Enable TX Pin
    SBUSOUT_UARTE->PSEL.TXD = (4 << UARTE_PSEL_TXD_PIN_Pos) | (1 << UARTE_PSEL_TXD_PORT_Pos);
    SBUSOUT_UARTE->PSEL.RXD = UARTE_PSEL_RXD_CONNECT_Disconnected << UARTE_PSEL_RXD_CONNECT_Pos;
    SBUSOUT_UARTE->PSEL.CTS = UARTE_PSEL_CTS_CONNECT_Disconnected << UARTE_PSEL_CTS_CONNECT_Pos;
    SBUSOUT_UARTE->PSEL.RTS = UARTE_PSEL_RTS_CONNECT_Disconnected << UARTE_PSEL_RTS_CONNECT_Pos;

    // Below uses two GPIOTE's to read the TX pin and cause it to be inverted on the RX pin

    // Setup as an input, when TX pin toggles state state causes the event to trigger and through
    // PPI toggle the output pin on next GPIOTE

    NRF_GPIOTE->CONFIG[SBUS_GPIOTE1] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (4 <<  GPIOTE_CONFIG_PSEL_Pos) |
            (1 << GPIOTE_CONFIG_PORT_Pos);

    //
    NRF_GPIOTE->CONFIG[SBUS_GPIOTE2] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (3 <<  GPIOTE_CONFIG_PSEL_Pos) |
            (1 << GPIOTE_CONFIG_PORT_Pos) |
            (1 << GPIOTE_CONFIG_OUTINIT_Pos);            // Initial value of pin low

    // On Compare equals Value, Toggle IO Pin
    NRF_PPI->CH[SBUSOUT_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUS_GPIOTE1];
    NRF_PPI->CH[SBUSOUT_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SBUS_GPIOTE2];

    // Enable PPI 11
    NRF_PPI->CHENSET = SBUSOUT_PPICH_MSK;

    // Enable the interrupt vector in IRQ Controller
    IRQ_CONNECT(UARTE1_IRQn, 2, SBUS_TX_Complete_Interrupt, NULL, 0);
    irq_enable(SBUSOUT_UARTE_IRQ);

    // Enable interupt in peripheral on end of transmission
    SBUSOUT_UARTE->INTENSET = UARTE_INTENSET_ENDTX_Msk;

    // Enable UART1
    SBUSOUT_UARTE->ENABLE = UARTE_ENABLE_ENABLE_Enabled << UARTE_ENABLE_ENABLE_Pos;
    isSBUSInit = true;

    // Initiate a transfer
    SBUSOUT_UARTE->TASKS_STARTTX = 1;
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

