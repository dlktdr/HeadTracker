/*
 * This file is part of the Head Tracker distribution (https://github.com/dlktdr/headtracker)
 * Copyright (c) 2022 Cliff Blackburn
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

static uint8_t serialDMATx[SERIAL_TX_SIZE];  // DMA Access Buffer Write

// In/Out Buffers
ringbuffer<uint8_t> serialRxBuf(SERIAL_RX_SIZE);
ringbuffer<uint8_t> serialTxBuf(SERIAL_TX_SIZE);

static bool invertTX = false;
static bool invertRX = false;

volatile bool isTransmitting = false;

void Serial_Start_TX(bool disableint = true) {}

void SerialTX_isr() {}

void SerialRX_isr() {}

int AuxSerial_Open(uint32_t baudrate, uint16_t prtset, uint8_t inversions)
{
  if (serialopened) return SERIAL_ALREADY_OPEN;

  AuxSerial_Close();  // Put the periferial in a good off state
}

void AuxSerial_Close()
{
  if (!serialopened) return;
  serialopened = false;
}

uint32_t AuxSerial_Write(const uint8_t* buffer , uint32_t len)
{
  if (serialTxBuf.getFree() < len) return SERIAL_BUFFER_FULL;
  serialTxBuf.write(buffer, len);
  Serial_Start_TX();
  return 0;
}

uint32_t AuxSerial_Read(uint8_t* buffer, uint32_t bufsize)
{
  return serialRxBuf.read(buffer, bufsize);
}

bool AuxSerial_Available() { return serialRxBuf.getOccupied() > 0; }