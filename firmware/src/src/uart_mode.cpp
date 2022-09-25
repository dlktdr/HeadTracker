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
static int64_t usduration;

// Switching modes, don't execute
volatile bool uartThreadRun = false;

void uart_init()
{
  uartThreadRun = true;
}

void uart_Thread()
{
  while (1) {
    // Check if bluetooth mode has changed
    if(curmode != trkset.getUartMode())
      UartSetMode((uartmodet)trkset.getUartMode());

    usduration = micros64();

    if (!uartThreadRun || pauseForFlash) {
      rt_sleep_ms(10);
      continue;
    }

    switch (curmode) {
      case UARTSBUSIO:
        SbusExec();
        break;
      case UARTCRSFIN:
        if(crsf)
          crsf->loop();
        break;
      default:
        break;
    }

    // Adjust sleep for a more accurate period
    /*usduration = micros64() - usduration;
    int64_t delay = UART_PERIOD;
    if (delay - usduration <
        delay * 0.7) {  // Took a long time. Will crash if sleep is too short
      rt_sleep_us(delay);
    } else {
      rt_sleep_us(delay - usduration);
    }

    rt_sleep_us((1.0 / (float)trkset.getSbRate()) * 1.0e6);*/
    rt_sleep_us(UART_PERIOD);
  }
}

void UartSetMode(uartmodet mode)
{
  // Requested same mode, just return
  if (mode == curmode) return;

  // Shut Down
  /*switch (curmode) {
    case :
      BTHeadStop();
      break;
    case BTPARARMT:
      BTRmtStop();
      btscanonly = false;
      break;
    case BTSCANONLY:
      BTRmtStop();
      btscanonly = false;
      break;
    default:
      break;
  }*/

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
  if (curmode == UARTDISABLE) return false;
  return true;
  //return connected;
}

uint16_t UartGetChannel(int chno)
{
  switch (curmode) {
    case UARTSBUSIO:
      //return SbusGetChannel(chno);
      break;
    case UARTCRSFIN:
      //return BTRmtGetChannel(chno);
      break;
    case BTSCANONLY:
      return 0;
    default:
      break;
  }

  return 0;
}

void UartSetChannel(int channel, const uint16_t value)
{
  switch (curmode) {
    case UARTSBUSIO:
      //BTHeadSetChannel(channel, value);
      break;
    case UARTCRSFOUT:
//      BTRmtSetChannel(channel, value);
      break;
    default:
      break;
  }
}
