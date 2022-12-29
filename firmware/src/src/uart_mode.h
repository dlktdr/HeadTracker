#pragma once

#include "defines.h"

typedef enum { UARTDISABLE = 0, UARTSBUSIO, UARTCRSFIN, UARTCRSFOUT } uartmodet;

extern uint32_t PacketCount;

void uart_init();
void uartTx_Thread();
void uartRx_Thread();

void UartSetMode(uartmodet mode);
uartmodet UartGetMode();

bool UartGetChannels(uint16_t channels[16]);
void UartSetChannels(uint16_t channels[16]);