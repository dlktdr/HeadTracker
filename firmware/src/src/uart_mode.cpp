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

#include "uart_mode.h"

#include <zephyr.h>

#include "CRSF/crsfin.h"
#include "CRSF/crsfout.h"
#include "SBUS/sbus.h"
#include "defines.h"
#include "io.h"
#include "log.h"
#include "soc_flash.h"
#include "trackersettings.h"

// s#define DEBUG_UART_RATE

// Globals
static uartmodet curmode = UARTDISABLE;
static uint32_t lastRead = 0;
static bool dataIsValid = false;

// Incoming channels
uint16_t uart_channels[18];

// Switching modes, don't execute
static struct k_poll_signal uartRxThreadRunSignal = K_POLL_SIGNAL_INITIALIZER(uartRxThreadRunSignal);
struct k_poll_event uartRxRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &uartRxThreadRunSignal),
};

static struct k_poll_signal uartTxThreadRunSignal = K_POLL_SIGNAL_INITIALIZER(uartTxThreadRunSignal);
struct k_poll_event uartTxRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &uartTxThreadRunSignal),
};



uint32_t PacketCount = 0;

void uart_init()
{
  for (int i = 0; i < 18; i++) {
    uart_channels[i] = 0;
  }
  lastRead = millis() + TrackerSettings::UART_ACTIVE_TIME;  // Start timed out
  k_poll_signal_raise(&uartTxThreadRunSignal, 1);
  k_poll_signal_raise(&uartRxThreadRunSignal, 1);
}

void uartRx_Thread()
{
  while (1) {
    k_poll(uartRxRunEvents, 1, K_FOREVER);
    rt_sleep_us(UART_PERIOD);

    if (pauseForFlash) {
      continue;
    }

    if (curmode != trkset.getUartMode()) UartSetMode((uartmodet)trkset.getUartMode());

#ifdef DEBUG_UART_RATE
    static int64_t mic = millis64() + 1000;
    if (mic < millis64()) {  // Every Second
      mic = millis64() + 1000;
      LOGI("UART Rate = %d", PacketCount);
      PacketCount = 0;
    }
#endif

    switch (curmode) {
      case UARTSBUSIO:
        // TODO.. mutex here, between this and read the data
        if (SbusReadChannels(uart_channels)) {
          lastRead = millis();
          dataIsValid = true;
        } else {  // No Data Read.. Wait a time, then mark stale
          if (millis() > lastRead + (TrackerSettings::UART_ACTIVE_TIME * 1000)) dataIsValid = false;
        }
        break;
      case UARTCRSFIN:
        if (crsfin) {
          crsfin->loop();
          dataIsValid = crsfin->isLinkUp();
          if (dataIsValid) {
            for (int i = 0; i < 16; i++) uart_channels[i] = crsfin->getChannel(i + 1);
          }
        }
        break;
      case UARTCRSFOUT:
        break;
      default:
        break;
    }
  }
}

void uartTx_Thread()
{
  while (1) {
    k_poll(uartTxRunEvents, 1, K_FOREVER);
    if (pauseForFlash) {
      rt_sleep_ms(1000);
      continue;
    }

    switch (curmode) {
      case UARTSBUSIO:
        SbusTx();
        rt_sleep_us((1.0 / (float)trkset.getSbusTxRate()) * 1.0e6);
        break;
      case UARTCRSFOUT:
        rt_sleep_us((1.0 / (float)trkset.getCrsfTxRate()) * 1.0e6);
        crsfout.sendRCFrameToFC();
        /*crsfout.AttitudeDataOut.pitch = 10;
        crsfout.AttitudeDataOut.roll = 30;
        crsfout.AttitudeDataOut.yaw += 1;
        crsfout.sendAttitideToFC();*/
        break;
      default:
        rt_sleep_ms(1000);
        break;
    }
  }
}

void UartSetMode(uartmodet mode)
{
  // Requested same mode, just return
  if (mode == curmode) return;

  for (int i = 0; i < 18; i++) {
    uart_channels[i] = 0;
  }

  // Start Up
  switch (mode) {
    case UARTSBUSIO:
      SbusInit();
      break;
    case UARTCRSFIN:
      if (crsfin) delete crsfin;
      CrsfInInit();
      break;
    case UARTCRSFOUT:
      CrsfOutInit();
      break;

    default:
      break;
  }

  curmode = mode;
}

uartmodet UartGetMode() { return curmode; }

bool UartGetConnected()
{
  switch (curmode) {
    case UARTSBUSIO:
    case UARTCRSFIN:
      if (dataIsValid) return true;
      break;
    default:
      break;
  }
  return false;
}

bool UartGetChannels(uint16_t channels[16])
{
  for (int i = 0; i < 16; i++) {
    if (dataIsValid) {
      channels[i] = uart_channels[i];
    } else {
      channels[i] = 0;
    }
  }

  // TODO: Mutex here
  return dataIsValid;
}

void UartSetChannels(uint16_t channels[16])
{
  switch (curmode) {
    case UARTSBUSIO:
      for (int i = 0; i < 16; i++) {
        channels[i] = (static_cast<float>(channels[i]) - TrackerSettings::PPM_CENTER) *
                          TrackerSettings::SBUS_SCALE +
                      TrackerSettings::SBUS_CENTER;
      }
      SbusWriteChannels(channels);
      break;
    case UARTCRSFOUT:
      crsfout.PackedRCdataOut.ch0 = US_to_CRSF(channels[0]);
      crsfout.PackedRCdataOut.ch1 = US_to_CRSF(channels[1]);
      crsfout.PackedRCdataOut.ch2 = US_to_CRSF(channels[2]);
      crsfout.PackedRCdataOut.ch3 = US_to_CRSF(channels[3]);
      crsfout.PackedRCdataOut.ch4 = US_to_CRSF(channels[4]);
      crsfout.PackedRCdataOut.ch5 = US_to_CRSF(channels[5]);
      crsfout.PackedRCdataOut.ch6 = US_to_CRSF(channels[6]);
      crsfout.PackedRCdataOut.ch7 = US_to_CRSF(channels[7]);
      crsfout.PackedRCdataOut.ch8 = US_to_CRSF(channels[8]);
      crsfout.PackedRCdataOut.ch9 = US_to_CRSF(channels[9]);
      crsfout.PackedRCdataOut.ch10 = US_to_CRSF(channels[10]);
      crsfout.PackedRCdataOut.ch11 = US_to_CRSF(channels[11]);
      crsfout.PackedRCdataOut.ch12 = US_to_CRSF(channels[12]);
      crsfout.PackedRCdataOut.ch13 = US_to_CRSF(channels[13]);
      crsfout.PackedRCdataOut.ch14 = US_to_CRSF(channels[14]);
      crsfout.PackedRCdataOut.ch15 = US_to_CRSF(channels[15]);
      crsfout.LinkStatistics.rf_Mode = RATE_LORA_100HZ_8CH;
      break;
    default:
      break;
  }
}
