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
#include <sys/ring_buffer.h>
#include "uarte_sbus.h"
#include "defines.h"
#include "io.h"
#include "trackersettings.h"
#include <nrfx.h>
#include <nrfx_uarte.h>

#define BAUD100000       0x0198EF80
#define CONF8E2          0x0000001E
#define SBUS_FRAME_LEN   25

#define SBUSIN1_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SBUSIN1_PPICH), _Msk )
#define SBUSIN2_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SBUSIN2_PPICH), _Msk )
#define SBUSIN3_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SBUSIN3_PPICH), _Msk )

#define SBUSOUT_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SBUSOUT_PPICH), _Msk )
#define SBUS_UARTE CONCAT(NRF_UARTE, SBUS_UARTE_CH)
#define SBUS_UARTE_IRQ CONCAT(CONCAT(UARTE, SBUS_UARTE_CH),_IRQn)

#define SBUSIN_TIMER CONCAT(NRF_TIMER, SBUSIN_TIMER_CH )
#define SBUSIN_TIMER_IRQNO CONCAT(CONCAT(TIMER, SBUSIN_TIMER_CH), _IRQn )
#define SBUSIN_TMRCOMP_CH_MSK CONCAT(CONCAT(TIMER_INTENSET_COMPARE,SBUSIN_TMRCOMP_CH),_Msk)

// Temp Pins Used, They are not connected on the NANO33BLE
#define SBUSOUT_TPIN  4
#define SBUSOUT_TPORT 1
#define SBUSIN_TPIN   15
#define SBUSIN_TPORT  1

// Actual Rx and Tx pins defined in platformio.ini
#define SBUS_RING_BUF 100

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
uint8_t localRXBuffer[SBUS_FRAME_LEN]; // Local Buffer

volatile bool isTransmitting=false;
volatile bool isSBUSInit=false;
volatile bool sbusoutinv=false;
volatile bool sbusininv=false;
volatile bool sbusinsof=false; // Start of Frame

uint8_t sbring_buffer[SBUS_RING_BUF]; // receive buffer
struct ring_buf sbinringbuf;
void SBUSIn_Process();

void SBUS_Thread()
{
    while(1) {
        if(isSBUSInit) {
            sbusoutinv = !trkset.invertedSBUSOut();
            SBUS_TX_Start();
            SBUSIn_Process();
        }
        k_usleep((1.0/(float)trkset.SBUSRate()) * 1.0e6);
    }
}

void SBUSIn_Process()
{
    static bool sof=false;
    static uint8_t sbcnt=0;

    uint8_t rbchar;
    while(ring_buf_get(&sbinringbuf, & rbchar,1)) {
        // Header, reset start of frame. Store in SBUS In Buffer
        if(rbchar == HEADER_) {
            sof = true;
            sbcnt = 0;
            localRXBuffer[sbcnt++] = rbchar;
        } else if(sof == true && sbcnt < SBUS_FRAME_LEN) { // Already got header, and less than frame length.
            localRXBuffer[sbcnt++] = rbchar;
            if(rbchar == FOOTER_) {
                // End of frame char and 25 bytes, should be valid data
                if(sbcnt == SBUS_FRAME_LEN) {
                    // PARSE THE DATA HERE
                    __NOP();
                    __NOP();
                    __NOP();
                    __NOP();
                    __NOP();
                    __NOP();
                // Enf of frame and not 25 bytes... invalid data
                } else {
                    sbcnt = 0;
                    sof=false;
                }
            }
        } else { // Invalid frame no footer found and > 25 bytes
            sof = false;
            sbcnt = 0;
        }
    }
}

void SBUSIn_SetInverted(bool sbusininv)
{
    if(isSBUSInit) {
        // Flip PPI Events to cause inversion
        NRF_PPI->CHENCLR = SBUSIN1_PPICH_MSK | SBUSIN2_PPICH_MSK;
        if(!sbusininv) {
            NRF_PPI->CH[SBUSIN1_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN0_GPIOTE];
            NRF_PPI->CH[SBUSIN2_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN1_GPIOTE];
        } else {
            NRF_PPI->CH[SBUSIN1_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN1_GPIOTE];
            NRF_PPI->CH[SBUSIN2_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN0_GPIOTE];
        }
        NRF_PPI->CHENSET = SBUSIN1_PPICH_MSK | SBUSIN2_PPICH_MSK;
    }
}

void SBUS_Interrupt(void *arg)
{
    // Transmission End
    if(SBUS_UARTE->EVENTS_ENDTX) {
        SBUS_UARTE->EVENTS_ENDTX = 0;
        isTransmitting = false;

        // Below is done to be sure every cycle invert pin is at the correct level

        // UARTE should be done transmitting here and pin should be high
        // Disable PPI 11
        NRF_PPI->CHENCLR = SBUSOUT_PPICH_MSK;

        // Disable and re-enable the SBUSOutput GPIOTE,
        // Prevents getting stuck in wrong level and allows GUI to switch inversion
        NRF_GPIOTE->CONFIG[SBUSOUT1_GPIOTE] = 0;
        uint32_t confreg = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                (SBUSOUT_PIN <<  GPIOTE_CONFIG_PSEL_Pos) |
                (SBUSOUT_PORT << GPIOTE_CONFIG_PORT_Pos);
        if(!sbusoutinv)
            confreg |= GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos;
        NRF_GPIOTE->CONFIG[SBUSOUT1_GPIOTE] = confreg;       // Initial value of pin low

        // Enable PPI
        NRF_PPI->CHENSET = SBUSOUT_PPICH_MSK;
    }

    // Receive Event
    if(SBUS_UARTE->EVENTS_RXDRDY) {
        SBUS_UARTE->EVENTS_RXDRDY = 0;
        // Write it to our local buffer for processing later & reset pointer
        ring_buf_put(&sbinringbuf, sbusDMARx, SBUS_UARTE->RXD.AMOUNT);
        SBUS_UARTE->RXD.PTR = (uint32_t)sbusDMARx;
    }

    // Error
    if(SBUS_UARTE->EVENTS_ERROR) {
        SBUS_UARTE->EVENTS_ERROR = 0;
        SBUS_UARTE->ERRORSRC = 0;
    }
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
    SBUS_UARTE->TASKS_STARTTX = 1;
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

    irq_disable(SBUS_UARTE_IRQ);

    // Disable UART1 + Interrupt & IRQ controller
    SBUS_UARTE->EVENTS_ENDTX = 0;
    SBUS_UARTE->ENABLE = UARTE_ENABLE_ENABLE_Disabled << UARTE_ENABLE_ENABLE_Pos;

    SBUS_UARTE->INTENCLR = UARTE_INTENSET_ENDTX_Msk;

    // Below uses two GPIOTE's to read the TX pin and cause it to be inverted temp pin

    // Setup as an input, when TX pin toggles state state causes the event to trigger and through
    // PPI toggle the output pin on next GPIOTE

    // P1.04 is unused and not connected onthe BLE    
    NRF_GPIOTE->CONFIG[SBUSOUT0_GPIOTE] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (SBUSOUT_TPIN <<  GPIOTE_CONFIG_PSEL_Pos) |
            (SBUSOUT_TPORT << GPIOTE_CONFIG_PORT_Pos);

    NRF_GPIOTE->CONFIG[SBUSOUT1_GPIOTE] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (SBUSOUT_PIN <<  GPIOTE_CONFIG_PSEL_Pos) |
            (SBUSOUT_PORT << GPIOTE_CONFIG_PORT_Pos);

    // Below uses three GPIOTE's for the Received SBUS Data + 2 PPI Channels
    // Done different from above because it was too easy to get stuck in the wrong level due to
    // signal noise just toggling on both event and tasks. This way will always be correct by
    // using an additional gpiote.
    //
    //     0 - Input Pin - Event HiToLo
    //     1 - Input Pin - Event LotoHi
    //     2 - Output Pin - Set / Clear Tasks set by two ppis from gpiote 0 & 1
    //
    // Output pin (1.05) is an unused pin on the NRF device, which is connected to the UARTE1 RX input

    NRF_GPIOTE->CONFIG[SBUSIN0_GPIOTE] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos) |
            (SBUSIN_PIN <<  GPIOTE_CONFIG_PSEL_Pos) |
            (SBUSIN_PORT << GPIOTE_CONFIG_PORT_Pos);

    NRF_GPIOTE->CONFIG[SBUSIN1_GPIOTE] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos) |
            (SBUSIN_PIN <<  GPIOTE_CONFIG_PSEL_Pos) |
            (SBUSIN_PORT << GPIOTE_CONFIG_PORT_Pos);

    // Output Pin. One gpiote sets it, one clears it
    nrf_gpio_cfg_input(SBUSIN_TPORT * 32 + SBUSIN_TPIN, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_watcher(SBUSIN_TPORT * 32 + SBUSIN_TPIN);

    NRF_GPIOTE->CONFIG[SBUSIN2_GPIOTE] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
            (SBUSIN_TPIN <<  GPIOTE_CONFIG_PSEL_Pos) |
            (SBUSIN_TPORT << GPIOTE_CONFIG_PORT_Pos);


    // Toggle output pin on every input pin toggle (SBUS out)
    NRF_PPI->CH[SBUSOUT_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSOUT0_GPIOTE];
    NRF_PPI->CH[SBUSOUT_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SBUSOUT1_GPIOTE];

    // Clear output pin on every input transition high (SBUS in)
    NRF_PPI->CH[SBUSIN1_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN0_GPIOTE];
    NRF_PPI->CH[SBUSIN1_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_SET[SBUSIN2_GPIOTE];

    // Set output pin on every input transition low (SBUS in)
    NRF_PPI->CH[SBUSIN2_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN1_GPIOTE];
    NRF_PPI->CH[SBUSIN2_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_CLR[SBUSIN2_GPIOTE];

    // Enable PPI (Both SBUS out + in)
    NRF_PPI->CHENSET = SBUSOUT_PPICH_MSK | SBUSIN1_PPICH_MSK | SBUSIN2_PPICH_MSK;

    // Baud 100000, 8E2
    // 25 Byte Data Send Length
    SBUS_UARTE->BAUDRATE = BAUD100000;
    SBUS_UARTE->CONFIG = CONF8E2;

    // DMA Access space for SBUS output
    SBUS_UARTE->TXD.PTR = (uint32_t)sbusDMATx;
    SBUS_UARTE->TXD.MAXCNT = sizeof(sbusDMATx);

    // DMA for SBUS input
    SBUS_UARTE->RXD.PTR = (uint32_t)sbusDMARx;
    SBUS_UARTE->RXD.MAXCNT = sizeof(sbusDMARx);

    // Enable TX + RX Pin, no flow control
    SBUS_UARTE->PSEL.TXD = (SBUSOUT_TPIN << UARTE_PSEL_TXD_PIN_Pos) | (SBUSOUT_TPORT << UARTE_PSEL_TXD_PORT_Pos);
    SBUS_UARTE->PSEL.RXD = (SBUSIN_TPIN << UARTE_PSEL_RXD_PIN_Pos) | (SBUSIN_TPORT << UARTE_PSEL_RXD_PORT_Pos);
    SBUS_UARTE->PSEL.CTS = UARTE_PSEL_CTS_CONNECT_Disconnected << UARTE_PSEL_CTS_CONNECT_Pos;
    SBUS_UARTE->PSEL.RTS = UARTE_PSEL_RTS_CONNECT_Disconnected << UARTE_PSEL_RTS_CONNECT_Pos;

    // Enable the interrupt vector in IRQ Controller
    IRQ_CONNECT(UARTE1_IRQn, 2, SBUS_Interrupt, NULL, 0);
    irq_enable(SBUS_UARTE_IRQ);

    // Enable interupt in peripheral on end of transmission
    SBUS_UARTE->INTENSET = UARTE_INTENSET_ENDTX_Msk | UARTE_INTENSET_RXDRDY_Msk | UARTE_INTENSET_ERROR_Msk;

    // Enable UARTE1
    SBUS_UARTE->ENABLE = UARTE_ENABLE_ENABLE_Enabled << UARTE_ENABLE_ENABLE_Pos;
    isSBUSInit = true;

    // Initiate a transfer
    SBUS_UARTE->TASKS_STARTTX = 1;

    // Start Receiving
    SBUS_UARTE->ERRORSRC = 0;
    SBUS_UARTE->TASKS_STARTRX = 1;

    ring_buf_init(&sbinringbuf, sizeof(sbring_buffer), sbring_buffer);
}

// Build Channel Data

/* FROM -----
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

