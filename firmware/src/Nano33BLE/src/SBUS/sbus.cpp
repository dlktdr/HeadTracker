/*
* Brian R Taylor
* brian.taylor@bolderflight.com
*
* Copyright (c) 2021 Bolder Flight Systems Inc
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the “Software”), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/

#include "Arduino.h"
#include "sbus.h"

// SBUS
rtos::Thread sbusthread(osPriorityHigh);
rtos::Mutex sbusmutex;
bfs::SbusTx sbus_tx(&Serial1);
std::array<uint16_t, 16> sbus_data;

void sbus_Init()
{
    sbusthread.start(sbusThread);
}

void sbusThread() {
    while(true) {
        sbusmutex.lock();
        sbus_tx.Write();
        sbusmutex.unlock();
        rtos::ThisThread::sleep_for(23);
    }
}

namespace bfs {

bool SbusRx::Begin() {
  state_ = 0;
  prev_byte_ = FOOTER_;
  #if defined(__MK20DX128__) || defined(__MK20DX256__)
    bus_->begin(BAUD_, SERIAL_8E1_RXINV_TXINV);
  #else
    bus_->begin(BAUD_, SERIAL_8N1); // ISSUE WITH ARDUINO ON BLE33, Set as 8N1, then override

  #endif
  /* flush the bus */
  bus_->flush();

  /* check communication */
  uint32_t timer_ms = 0;
  while (timer_ms < TIMEOUT_MS_) {
    if (Read()) {
      if (!failsafe() && !lost_frame()) {
        return true;
      }
    }
  }
  return false;
}

bool SbusRx::Read() {
  bool status = false;
  /*  Parse through to the most recent packet if we've fallen behind */
  while (bus_->available()) {
    if (Parse()) {
    /* Grab the channel data */
      ch_[0]  = static_cast<uint16_t>((buf_[1]       | buf_[2]  << 8) & 0x07FF);
      ch_[1]  = static_cast<uint16_t>((buf_[2]  >> 3 | buf_[3]  << 5) & 0x07FF);
      ch_[2]  = static_cast<uint16_t>((buf_[3]  >> 6 | buf_[4]  << 2  |  buf_[5] << 10) & 0x07FF);
      ch_[3]  = static_cast<uint16_t>((buf_[5]  >> 1 | buf_[6]  << 7) & 0x07FF);
      ch_[4]  = static_cast<uint16_t>((buf_[6]  >> 4 | buf_[7]  << 4) & 0x07FF);
      ch_[5]  = static_cast<uint16_t>((buf_[7]  >> 7 | buf_[8]  << 1  |   buf_[9] << 9) & 0x07FF);
      ch_[6]  = static_cast<uint16_t>((buf_[9]  >> 2 | buf_[10] << 6) & 0x07FF);
      ch_[7]  = static_cast<uint16_t>((buf_[10] >> 5 | buf_[11] << 3) & 0x07FF);
      ch_[8]  = static_cast<uint16_t>((buf_[12]      | buf_[13] << 8) & 0x07FF);
      ch_[9]  = static_cast<uint16_t>((buf_[13] >> 3 | buf_[14] << 5) & 0x07FF);
      ch_[10] = static_cast<uint16_t>((buf_[14] >> 6 | buf_[15] << 2  |   buf_[16] << 10) & 0x07FF);
      ch_[11] = static_cast<uint16_t>((buf_[16] >> 1 | buf_[17] << 7) & 0x07FF);
      ch_[12] = static_cast<uint16_t>((buf_[17] >> 4 | buf_[18] << 4) & 0x07FF);
      ch_[13] = static_cast<uint16_t>((buf_[18] >> 7 | buf_[19] << 1  |   buf_[20] << 9) & 0x07FF);
      ch_[14] = static_cast<uint16_t>((buf_[20] >> 2 | buf_[21] << 6) & 0x07FF);
      ch_[15] = static_cast<uint16_t>((buf_[21] >> 5 | buf_[22] << 3) & 0x07FF);
      /* Channel 17 */
      ch17_ = buf_[23] & CH17_;
      /* Channel 18 */
      ch18_ = buf_[23] & CH18_;
      /* Grab the lost frame */
      lost_frame_ = buf_[23] & LOST_FRAME_;
      /* Grab the failsafe */
      failsafe_ = buf_[23] & FAILSAFE_;
      /* Set the status */
      status = true;
    }
  }
  return status;
}
bool SbusRx::Parse() {
  while (bus_->available()) {
    uint8_t c = bus_->read();
    if (state_ == 0) {
      if ((c == HEADER_) && ((prev_byte_ == FOOTER_) ||
         ((prev_byte_ & 0x0F) == FOOTER2_))) {
        buf_[state_] = c;
        state_++;
      } else {
        state_ = 0;
      }
    } else {
      if (state_ < LEN_) {
        buf_[state_] = c;
        state_++;
      } else {
        state_ = 0;
        if ((buf_[LEN_ - 1] == FOOTER_) ||
           ((buf_[LEN_ - 1] & 0x0F) == FOOTER2_)) {
          return true;
        } else {
          return false;
        }
      }
    }
    prev_byte_ = c;
  }
  return false;
}

/* Needed for emulating two stop bytes on Teensy 3.0 and 3.1/3.2 */
#if defined(__MK20DX128__) || defined(__MK20DX256__)
namespace {
  IntervalTimer serial_timer;
  HardwareSerial *sbus_bus;
  uint8_t sbus_packet[25];
  volatile int send_index;
  void SendByte() {
    if (send_index < 25) {
      sbus_bus->write(sbus_packet[send_index]);
      send_index++;
    } else {
      serial_timer.end();
      send_index = 0;
    }
  }
}  // namespace
#endif

#define UARTE0_BASE_ADDR            0x40002000  // As per nRF52840 Product spec - UARTE
#define UART_CONFIG_REG_OFFSET      0x56C
#define UART_BAUDRATE_REG_OFFSET    0x524 // As per nRF52840 Product spec - UARTE
#define UART0_BAUDRATE_REGISTER     (*(( unsigned int *)(UARTE0_BASE_ADDR + UART_BAUDRATE_REG_OFFSET)))
#define UART0_CONFIG_REGISTER       (*(( unsigned int *)(UARTE0_BASE_ADDR + UART_CONFIG_REG_OFFSET)))
#define BAUD100000                   0x0198EF80
#define CONF8E2                      0x0000001E

void SbusTx::Begin() {
  #if defined(__MK20DX128__) || defined(__MK20DX256__)
    bus_->begin(BAUD_, SERIAL_8E1_RXINV_TXINV);
  #else
    bus_->begin(BAUD_, SERIAL_8N1);

    // Issue with Arduino, Cannot set 8E2 without crashing mbed.
    // this manually sets the config regs to 100000 baud 8E2
    UART0_BAUDRATE_REGISTER = BAUD100000;
    UART0_CONFIG_REGISTER   = CONF8E2;

    // Below uses two GPIOTE's to read the TX pin and cause it to be inverted on the RX pin
    int txpin=3;
    int txport=1;
    int invtxpin=10;
    int invtxport=1;
    // Setup as an input, when TX pin changes state causes
    NRF_GPIOTE->CONFIG[4] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
            (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
            (txpin <<  GPIOTE_CONFIG_PSEL_Pos) |
            (txport << GPIOTE_CONFIG_PORT_Pos);

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
  #endif
}
void SbusTx::Write() {
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

  #if defined(__MK20DX128__) || defined(__MK20DX256__)
    /*
    * Use ISR to send byte at a time,
    * 130 us between bytes to emulate 2 stop bits
    */
    __disable_irq();
    memcpy(sbus_packet, tx_buffer_, sizeof(sbus_packet));
    __enable_irq();
    serial_timer.priority(255);
    serial_timer.begin(SendByte, 130);
  #else
    bus_->write(buf_, sizeof(buf_));
  #endif
}

}  // namespace bfs