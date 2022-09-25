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

#include "auxserial.h"

#include <nrfx.h>
#include <nrfx_ppi.h>
#include <nrfx_uarte.h>
#include <zephyr.h>

#include "defines.h"
#include "ringbuffer.h"


static bool serialopened = false;

#define SERIALIN1_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SERIALIN1_PPICH), _Msk)
#define SERIALIN2_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SERIALIN2_PPICH), _Msk)
#define SERIALOUT_PPICH_MSK CONCAT(CONCAT(PPI_CHENSET_CH, SERIALOUT_PPICH), _Msk)

#define SERIAL_UARTE CONCAT(NRF_UARTE, SERIAL_UARTE_CH)
#define SERIAL_UARTE_IRQ CONCAT(CONCAT(UARTE, SERIAL_UARTE_CH), _IRQn)

// Aux Serial Output Pins
#define AUXSERIALIN_PIN  PIN_TO_NRFPIN(PinNumber[IO_RX])
#define AUXSERIALIN_PORT PIN_TO_NRFPORT(PinNumber[IO_RX])

// Aux Serial Output Pins
#define AUXSERIALOUT_PIN  PIN_TO_NRFPIN(PinNumber[IO_TX])
#define AUXSERIALOUT_PORT PIN_TO_NRFPORT(PinNumber[IO_TX])

// Temp Pin, They are not broken out on the NANO33BLE
#define SERIALOUT_TPIN  PIN_TO_NRFPIN(PinNumber[IO_TXINV])
#define SERIALOUT_TPORT PIN_TO_NRFPORT(PinNumber[IO_TXINV])

// Pin RXINVO + RXINVI must be soldered together for inverted SBUS input
#define SERIALIN_TPIN_OUT  PIN_TO_NRFPIN(PinNumber[IO_RXINVO])
#define SERIALIN_TPORT_OUT PIN_TO_NRFPORT(PinNumber[IO_RXINVO])

// This is the pin the UARTE RX input is hooked up to.
#define SERIALIN_TPIN_IN  PIN_TO_NRFPIN(PinNumber[IO_RXINVI])
#define SERIALIN_TPORT_IN PIN_TO_NRFPORT(PinNumber[IO_RXINVI])

static uint8_t serialDMATx[SERIAL_TX_SIZE];  // DMA Access Buffer Write

// In/Out Buffers
ringbuffer<uint8_t> serialRxBuf(SERIAL_RX_SIZE);
ringbuffer<uint8_t> serialTxBuf(SERIAL_TX_SIZE);

static bool invertTX = false;
static bool invertRX = false;

volatile bool isTransmitting = false;

void Serial_Start_TX()
{
  size_t len = MIN(serialTxBuf.getOccupied(), sizeof(serialDMATx));
  if (len == 0) return;

  // If currently sending data, ISR will load the next payload
  //  otherwise, load the buffer and initiat the transfer
  irq_disable(SERIAL_UARTE_IRQ);
  if(isTransmitting == false) {
    isTransmitting = true;
    serialTxBuf.read(serialDMATx, len);
    SERIAL_UARTE->EVENTS_ENDTX = 0;
    SERIAL_UARTE->TXD.MAXCNT = len;
    SERIAL_UARTE->TASKS_STARTTX = 1;
  }
  irq_enable(SERIAL_UARTE_IRQ);
}

void SerialTX_isr()
{
  // Transmission End
  if (SERIAL_UARTE->EVENTS_ENDTX) {
    SERIAL_UARTE->EVENTS_ENDTX = 0;

    // Below is done to be sure every cycle invert pin is at the correct level
    NRF_PPI->CHENCLR = SERIALOUT_PPICH_MSK;

    // Disable and re-enable the SBUSOutput GPIOTE,
    // Prevents getting stuck in wrong level and allows GUI to switch inversion
    NRF_GPIOTE->CONFIG[SERIALOUT1_GPIOTE] = 0;
    uint32_t confreg = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                       (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                       (AUXSERIALOUT_PIN << GPIOTE_CONFIG_PSEL_Pos) |
                       (AUXSERIALOUT_PORT << GPIOTE_CONFIG_PORT_Pos);
    if (!invertTX) confreg |= GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos;
    NRF_GPIOTE->CONFIG[SERIALOUT1_GPIOTE] = confreg;  // Initial value of pin low

    // Enable PPI
    NRF_PPI->CHENSET = SERIALOUT_PPICH_MSK;

    // If there is more data, load and send it
    size_t len = MIN(serialTxBuf.getOccupied(), sizeof(serialDMATx));
    if (len > 0) {
      serialTxBuf.read(serialDMATx, len);
      SERIAL_UARTE->TXD.MAXCNT = len;
      SERIAL_UARTE->TASKS_STARTTX = 1;
    } else {
      isTransmitting = false;
    }
  }
}

void SerialRX_isr()
{
  ISR_DIRECT_HEADER();
  NRF_UART0->EVENTS_RXDRDY = 0;
  uint8_t rxv = (uint8_t)NRF_UART0->RXD;
  serialRxBuf.write(&rxv, 1);
  ISR_DIRECT_FOOTER(1);
}

int AuxSerial_Open(uint32_t baudrate, uint16_t prtset, uint8_t inversions)
{
  if (serialopened) return SERIAL_ALREADY_OPEN;

  AuxSerial_Close();  // Put the periferial in a good off state

  // Set inversion from flags
  invertTX = false;
  invertRX = false;
  if (inversions & CONFINV_TX) invertTX = true;
  if (inversions & CONFINV_RX) invertRX = true;

  // Setup UARTE for TX
  SERIAL_UARTE->BAUDRATE = baudrate;
  SERIAL_UARTE->CONFIG = prtset;
  SERIAL_UARTE->TXD.PTR = (uint32_t)serialDMATx;
  SERIAL_UARTE->TXD.MAXCNT = sizeof(serialDMATx);
  SERIAL_UARTE->PSEL.TXD =
      (SERIALOUT_TPIN << UARTE_PSEL_TXD_PIN_Pos) | (SERIALOUT_TPORT << UARTE_PSEL_TXD_PORT_Pos);
  SERIAL_UARTE->PSEL.RXD = UARTE_PSEL_RXD_CONNECT_Disconnected << UARTE_PSEL_RXD_CONNECT_Pos;
  SERIAL_UARTE->PSEL.CTS = UARTE_PSEL_CTS_CONNECT_Disconnected << UARTE_PSEL_CTS_CONNECT_Pos;
  SERIAL_UARTE->PSEL.RTS = UARTE_PSEL_RTS_CONNECT_Disconnected << UARTE_PSEL_RTS_CONNECT_Pos;

  // Setup UART0 for RX
  NRF_UART0->BAUDRATE = baudrate;
  NRF_UART0->CONFIG = prtset;

  // Below uses two GPIOTE's to read the TX pin and cause it to be inverted temp pin
  // Setup as an input, when TX pin toggles state state causes the event to trigger and through
  // PPI toggle the output pin on next GPIOTE
  // P1.04 is unused and not connected onthe BLE, this is the output inversion is done on

  NRF_GPIOTE->CONFIG[SERIALOUT0_GPIOTE] =
      (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
      (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
      (SERIALOUT_TPIN << GPIOTE_CONFIG_PSEL_Pos) | (SERIALOUT_TPORT << GPIOTE_CONFIG_PORT_Pos);

  NRF_GPIOTE->CONFIG[SERIALOUT1_GPIOTE] =
      (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
      (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
      (AUXSERIALOUT_PIN << GPIOTE_CONFIG_PSEL_Pos) | (AUXSERIALOUT_PORT << GPIOTE_CONFIG_PORT_Pos);

  // Toggle output pin on every input pin toggle (SBUS out)
  NRF_PPI->CH[SERIALOUT_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SERIALOUT0_GPIOTE];
  NRF_PPI->CH[SERIALOUT_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[SERIALOUT1_GPIOTE];

  NRF_PPI->CHENSET = SERIALOUT_PPICH_MSK;

  // Setup UART RX

  // Below uses three GPIOTE's for the Received SBUS Data + 2 PPI Channels
  // Done different from above because it was too easy to get stuck in the wrong level due to
  // signal noise just toggling on both event and tasks. This way will always be correct by
  // using an additional gpiote.
  //
  //     0 - Input Pin - Event HiToLo
  //     1 - Input Pin - Event LotoHi
  //     2 - Output Pin - Set / Clear Tasks set by two ppis from gpiote 0 & 1
  //
  // Output pin D5(1.13) is used as the output pin, which will be the inverted RX Pin Data
  if (invertRX) {
    NRF_GPIOTE->CONFIG[SERIALIN0_GPIOTE] =
        (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
        (GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos) |
        (AUXSERIALIN_PIN << GPIOTE_CONFIG_PSEL_Pos) | (AUXSERIALIN_PORT << GPIOTE_CONFIG_PORT_Pos);

    NRF_GPIOTE->CONFIG[SERIALIN1_GPIOTE] =
        (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
        (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos) |
        (AUXSERIALIN_PIN << GPIOTE_CONFIG_PSEL_Pos) | (AUXSERIALIN_PORT << GPIOTE_CONFIG_PORT_Pos);

    // Output Pin. One gpiote sets it, one clears it
    NRF_GPIOTE->CONFIG[SERIALIN2_GPIOTE] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                           (SERIALIN_TPIN_OUT << GPIOTE_CONFIG_PSEL_Pos) |
                                           (SERIALIN_TPORT_OUT << GPIOTE_CONFIG_PORT_Pos);

    // Clear output pin on every input transition high (SBUS in)
    NRF_PPI->CH[SERIALIN1_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SERIALIN0_GPIOTE];
    NRF_PPI->CH[SERIALIN1_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_SET[SERIALIN2_GPIOTE];
    // NRF_PPI->FORK[SBUSIN1_PPICH].TEP = (uint32_t)&SBUSIN_TIMER->TASKS_START; // Start Timer on
    // HitoLow

    // Set output pin on every input transition low (SBUS in)
    NRF_PPI->CH[SERIALIN2_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SERIALIN1_GPIOTE];
    NRF_PPI->CH[SERIALIN2_PPICH].TEP = (uint32_t)&NRF_GPIOTE->TASKS_CLR[SERIALIN2_GPIOTE];
    // NRF_PPI->FORK[SBUSIN2_PPICH].TEP = (uint32_t)&SBUSIN_TIMER->TASKS_CLEAR; // Reset Timer on
    // LowtoHi
    NRF_PPI->CHENSET = SERIALIN1_PPICH_MSK | SERIALIN2_PPICH_MSK;

    NRF_UART0->PSEL.RXD = (SERIALIN_TPIN_IN << UARTE_PSEL_RXD_PIN_Pos) |
                          (SERIALIN_TPORT_IN << UARTE_PSEL_RXD_PORT_Pos);
  } else {
    NRF_GPIOTE->CONFIG[SERIALIN0_GPIOTE] = 0;
    NRF_GPIOTE->CONFIG[SERIALIN1_GPIOTE] = 0;
    NRF_GPIOTE->CONFIG[SERIALIN2_GPIOTE] = 0;
    NRF_PPI->CHENCLR = SERIALIN1_PPICH_MSK | SERIALIN2_PPICH_MSK;

    NRF_UART0->PSEL.RXD =
        (AUXSERIALIN_PIN << UARTE_PSEL_RXD_PIN_Pos) | (AUXSERIALIN_PORT << UARTE_PSEL_RXD_PORT_Pos);
    // Set the pin D5 + D6 back to floating inputs
    //        nrf_gpio_cfg_input(SERIALIN_TPORT_OUT * 32 + SERIALIN_TPIN_OUT,NRF_GPIO_PIN_NOPULL);
    //        nrf_gpio_cfg_input(SERIALIN_TPORT_IN * 32 + SERIALIN_TPIN_IN, NRF_GPIO_PIN_NOPULL);
  }

  // Start Timer
  //    Timer is used to know when the SBUS line is idle, if idle for 1ms then force stop the UARTE
  //    to read the buffer.
  /*SBUSIN_TIMER->PRESCALER = 4; // 16Mhz/2^4 = 1Mhz = 1us Resolution, 1.048s Max@32bit
  SBUSIN_TIMER->MODE = TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos;
  SBUSIN_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
  SBUSIN_TIMER->CC[0] = 500; // 0.5ms of no data, stop uarte for reading
  SBUSIN_TIMER->SHORTS = TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos; //
  Stop timer on compare equals

  // On timer compare, stop UART
  NRF_PPI->CH[SBUSINTMR_PPICH].EEP = (uint32_t)&SBUSIN_TIMER->EVENTS_COMPARE[0]; // On Compare
  Event, NRF_PPI->CH[SBUSINTMR_PPICH].TEP = (uint32_t)&SBUS_UARTE->TASKS_STOPRX; // Stop Receive*/

  // SBUSIN_TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Msk; // Enable Compare 0 Interrupt... if PPI
  // not good

  // Baud 100000, 8E2
  // 25 Byte Data Send Length

  // DMA for SBUS input
  // SBUS_UARTE->RXD.PTR = (uint32_t)sbusDMARx;
  // SBUS_UARTE->RXD.MAXCNT = sizeof(sbusDMARx);

  // Enable TX Pin, no flow control

  // Use UART for receive... Gave up on the UARTE RX DMA method.. maybe will re-visit one day. Would
  // be less interrupts, but the ISR is pretty simple.

  // Enable the interrupt vector in IRQ Controller
  IRQ_CONNECT(SERIAL_UARTE_IRQ, 2, SerialTX_isr, NULL, 0);
  irq_enable(SERIAL_UARTE_IRQ);
  // Enable interupt on end of transmission
  SERIAL_UARTE->INTENSET =
      UARTE_INTENSET_ENDTX_Msk;  // | UARTE_INTENSET_RXDRDY_Msk | UARTE_INTENSET_ERROR_Msk;
  // Enable UARTE1
  SERIAL_UARTE->ENABLE = UARTE_ENABLE_ENABLE_Enabled << UARTE_ENABLE_ENABLE_Pos;

  IRQ_DIRECT_CONNECT(UART0_IRQn, 1, SerialRX_isr, 0);
  // IRQ_CONNECT(UART0_IRQn, 0, SBUSRX_Interrupt, NULL, 0);
  irq_enable(UART0_IRQn);

  // Enable UART Interrupt and UART
  NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Msk;
  NRF_UART0->ENABLE = UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos;
  NRF_UART0->TASKS_STARTRX = 1;

  // Start Receiving
  // SBUS_UARTE->ERRORSRC = 0x0F; // Clear any errors
  // SBUS_UARTE->TASKS_STARTRX = 1;

  // ring_buf_init(&sbinringbuf, sizeof(sbring_buffer), sbring_buffer);
  serialopened = true;
  return 0;
}

// Disable PPI's
/*NRF_PPI->CHENCLR = SBUSIN1_PPICH_MSK | SBUSIN2_PPICH_MSK;
// Stop UARTE receiver
SBUS_UARTE->TASKS_STOPRX = 1;

// If SBUS inverted (NORMAL UART), Disable GPIOTE's toggle output on TempOut pin D5(1.13)
// Connect UARTE RX directly to the RXpin (1.10)
if(sbusininv) {
    // Disable the GPIOTE
    NRF_GPIOTE->CONFIG[SBUSIN2_GPIOTE] = 0;
    // Set the UARTE to use the RX Pin directly
    NRF_UART0->PSEL.RXD = (AUXSERIALIN_PIN << UARTE_PSEL_RXD_PIN_Pos) | (AUXSERIALIN_PORT <<
UARTE_PSEL_RXD_PORT_Pos);
    // Set the pin D5 + D6 back to floating inputs
    nrf_gpio_cfg_input(SBUSIN_TPORT_OUT * 32 + SBUSIN_TPIN_OUT,NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(SBUSIN_TPORT_IN * 32 + SBUSIN_TPIN_IN, NRF_GPIO_PIN_NOPULL);

// If Inverted use the GPIOTE to output the inverted RX data on D5(1.13), connect the UARTERX to
D6(1.14) -- *** USER MUST SOLDER TOGETHER } else {
    // Enable the GPIOTE, Will force set to an output
    NRF_GPIOTE->CONFIG[SBUSIN2_GPIOTE] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
        (SBUSIN_TPIN_OUT <<  GPIOTE_CONFIG_PSEL_Pos) |
        (SBUSIN_TPORT_OUT << GPIOTE_CONFIG_PORT_Pos);

    // D6 Is the Input, set as an input. -
    nrf_gpio_cfg_input(SBUSIN_TPORT_IN * 32 + SBUSIN_TPIN_IN, NRF_GPIO_PIN_NOPULL);

    // Set the SBUSRX Pin to D6(1.14)
    NRF_UART0->PSEL.RXD = (SBUSIN_TPIN_IN << UARTE_PSEL_RXD_PIN_Pos) | (SBUSIN_TPORT_IN <<
UARTE_PSEL_RXD_PORT_Pos);

    // Enable the PPI's
    NRF_PPI->CH[SBUSIN1_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN0_GPIOTE];
    NRF_PPI->CH[SBUSIN2_PPICH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[SBUSIN1_GPIOTE];
    NRF_PPI->CHENSET = SBUSIN1_PPICH_MSK | SBUSIN2_PPICH_MSK;
}

// Start Receiver
SBUS_UARTE->ERRORSRC = 0x0F; // Clear any errors
SBUS_UARTE->TASKS_STARTRX = 1;
*/

void AuxSerial_Close()
{
  // Stop Interrupts
  irq_disable(UART0_IRQn);
  irq_disable(SERIAL_UARTE_IRQ);

  // Disable PPI's
  NRF_PPI->CHENCLR = SERIALIN1_PPICH_MSK | SERIALIN2_PPICH_MSK | SERIALOUT_PPICH_MSK;

  // Disable UART
  NRF_UART0->ENABLE = 0;
  NRF_UART0->TASKS_STOPRX = 1;
  NRF_UART0->TASKS_STOPTX = 1;
  NRF_UART0->CONFIG = 0;
  NRF_UART0->PSEL.TXD = UART_PSEL_TXD_CONNECT_Disconnected << UART_PSEL_TXD_CONNECT_Pos;
  NRF_UART0->PSEL.RXD = UART_PSEL_RXD_CONNECT_Disconnected << UART_PSEL_RXD_CONNECT_Pos;
  NRF_UART0->PSEL.CTS = UART_PSEL_CTS_CONNECT_Disconnected << UART_PSEL_CTS_CONNECT_Pos;
  NRF_UART0->PSEL.RTS = UART_PSEL_RTS_CONNECT_Disconnected << UART_PSEL_RTS_CONNECT_Pos;
  NRF_UART0->INTENCLR = 0xFFFFFFFF;
  NRF_UART0->ERRORSRC = 0;

  // Disable UARTE
  SERIAL_UARTE->ENABLE = 0;
  SERIAL_UARTE->TASKS_STOPRX = 1;
  SERIAL_UARTE->TASKS_STOPTX = 1;
  SERIAL_UARTE->INTENCLR = 0xFFFFFFFF;
  SERIAL_UARTE->PSEL.CTS = UARTE_PSEL_CTS_CONNECT_Disconnected
                           << UARTE_PSEL_CTS_CONNECT_Disconnected;
  SERIAL_UARTE->PSEL.RTS = UARTE_PSEL_RTS_CONNECT_Disconnected
                           << UARTE_PSEL_RTS_CONNECT_Disconnected;
  SERIAL_UARTE->PSEL.TXD = UARTE_PSEL_TXD_CONNECT_Disconnected
                           << UARTE_PSEL_TXD_CONNECT_Disconnected;
  SERIAL_UARTE->PSEL.RXD = UARTE_PSEL_RXD_CONNECT_Disconnected
                           << UARTE_PSEL_RXD_CONNECT_Disconnected;
  SERIAL_UARTE->EVENTS_ENDTX = 0;
  SERIAL_UARTE->EVENTS_ENDRX = 0;
  SERIAL_UARTE->ERRORSRC = 0;

  // Disable GPIOTE
  NRF_GPIOTE->CONFIG[SERIALIN0_GPIOTE] = 0;
  NRF_GPIOTE->CONFIG[SERIALIN1_GPIOTE] = 0;
  NRF_GPIOTE->CONFIG[SERIALIN2_GPIOTE] = 0;
  NRF_GPIOTE->CONFIG[SERIALOUT1_GPIOTE] = 0;
  NRF_GPIOTE->CONFIG[SERIALOUT1_GPIOTE] = 0;

  serialopened = false;
}

uint32_t AuxSerial_Write(const uint8_t *buffer, uint32_t len)
{
  if (serialTxBuf.getFree() < len) return SERIAL_BUFFER_FULL;
  serialTxBuf.write(buffer, len);
  Serial_Start_TX();
  return 0;
}

uint32_t AuxSerial_Read(uint8_t *buffer, uint32_t bufsize)
{
  return serialRxBuf.read(buffer, bufsize);
}

bool AuxSerial_Available() { return serialRxBuf.getOccupied() > 0; }