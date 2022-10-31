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

#pragma once

#include <stdint.h>

#define SERIAL_TX_SIZE 512
#define SERIAL_RX_SIZE 512

#define BAUD1200 UART_BAUDRATE_BAUDRATE_Baud1200
#define BAUD2400 UART_BAUDRATE_BAUDRATE_Baud2400
#define BAUD4800 UART_BAUDRATE_BAUDRATE_Baud4800
#define BAUD9600 UART_BAUDRATE_BAUDRATE_Baud9600
#define BAUD11400 UART_BAUDRATE_BAUDRATE_Baud14400
#define BAUD19200 UART_BAUDRATE_BAUDRATE_Baud19200
#define BAUD28800 UART_BAUDRATE_BAUDRATE_Baud28800
#define BAUD31250 UART_BAUDRATE_BAUDRATE_Baud31250
#define BAUD38400 UART_BAUDRATE_BAUDRATE_Baud38400
#define BAUD56000 UART_BAUDRATE_BAUDRATE_Baud56000
#define BAUD57600 UART_BAUDRATE_BAUDRATE_Baud57600
#define BAUD76800 UART_BAUDRATE_BAUDRATE_Baud76800
#define BAUD100000 0x0198EF80
#define BAUD115200 UART_BAUDRATE_BAUDRATE_Baud115200
#define BAUD230400 UART_BAUDRATE_BAUDRATE_Baud230400
#define BAUD250000 UART_BAUDRATE_BAUDRATE_Baud250000
#define BAUD400000 0x06666666
#define BAUD420000 0x06B851EB
#define BAUD460800 UART_BAUDRATE_BAUDRATE_Baud460800
#define BAUD921600 UART_BAUDRATE_BAUDRATE_Baud921600
#define BAUD1000000 UART_BAUDRATE_BAUDRATE_Baud1M

#define CONF8N1 0x00000000
#define CONF8E2 0x0000001E
#define CONFINV_TX (1 << 0)
#define CONFINV_RX (1 << 1)

enum SerialStatus {
  SERIAL_OK = 0,
  SERIAL_ERROR = -1,
  SERIAL_ALREADY_OPEN = -2,
  SERIAL_BUFFER_FULL = -3,
};

int AuxSerial_Open(uint32_t baudrate, uint16_t settings, uint8_t inversions = 0);
bool AuxSerial_Available();
void AuxSerial_Close();
uint32_t AuxSerial_Write(const uint8_t* buffer, uint32_t len);
uint32_t AuxSerial_Read(uint8_t* buffer, uint32_t bufsize);