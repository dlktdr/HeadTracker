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
#include "defines.h"
#include "soc_flash.h"
#include "trackersettings.h"

#include "io.h"
#include "log.h"
#include "CRSF/crsf.h"
#include "SBUS/sbus.h"

// Globals
static uartmodet curmode = UARTDISABLE;
static uint32_t lastRead;
static bool dataIsValid = false;

// Incoming channels
uint16_t uart_channels[18];

// Switching modes, don't execute
volatile bool uartThreadRun = false;

void uart_init()
{
  for(int i=0; i < 18; i++) {
    uart_channels[i] = 0;
  }
  lastRead = millis() + TrackerSettings::UART_ACTIVE_TIME; // Start timed out
  uartThreadRun = true;
}

void uartRx_Thread()
{
  while (1) {
    rt_sleep_us(UART_PERIOD);
    if (!uartThreadRun || pauseForFlash) {
      continue;
    }

    if(curmode != trkset.getUartMode())
      UartSetMode((uartmodet)trkset.getUartMode());

    switch (curmode) {
      case UARTSBUSIO:
        // TODO.. mutex here, between this and read the data
        if(SbusReadChannels(uart_channels)) {
          lastRead = millis();
          dataIsValid = true;
        } else {
          if(lastRead + TrackerSettings::UART_ACTIVE_TIME < millis())
            dataIsValid = true;
          else
            dataIsValid = false;
        }
        break;
      case UARTCRSFIN:
        if(crsf) {
          crsf->loop();
          dataIsValid = crsf->isLinkUp();
          if(dataIsValid) {
            for(int i=0; i < 16; i++)
              uart_channels[i] = crsf->getChannel(i+1);
          }
        }
        break;
      default:
        break;
    }
  }
}

void uartTx_Thread()
{
  while(1) {
    rt_sleep_us((1.0 / (float)trkset.getUartTxRate()) * 1.0e6);
    if (!uartThreadRun || pauseForFlash) {
      continue;
    }

    switch (curmode) {
      case UARTSBUSIO:
        SbusTx();
        break;
      case UARTCRSFOUT:
        // Todo send a CRSF packet
        break;
      default:
        break;
    }
  }
}

void UartSetMode(uartmodet mode)
{
  // Requested same mode, just return
  if (mode == curmode) return;

  for(int i=0; i < 18; i++) {
    uart_channels[i] = 0;
  }

  // Start Up
  switch (mode) {
    case UARTSBUSIO:
      SbusInit();
      break;
    case UARTCRSFIN:
      if(crsf)
        delete crsf;
      CrsfInInit();
      break;
    default:
      break;
  }

  uartThreadRun = true;
  curmode = mode;
}

uartmodet UartGetMode() { return curmode; }

bool UartGetConnected()
{
  switch (curmode) {
    case UARTSBUSIO:
    case UARTCRSFIN:
      if(dataIsValid)
        return true;
      break;
    default:
      break;
  }
  return false;
}

bool UartGetChannels(uint16_t channels[16])
{
  for(int i=0; i < 16; i++) {
    if(dataIsValid) {
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
      SbusWriteChannels(channels);
      break;
    case UARTCRSFOUT:

      break;
    default:
      break;
  }
}
