#pragma once

#include "defines.h"

typedef enum { UARTDISABLE = 0, UARTSBUSIO, UARTCRSFIN, UARTCRSFOUT } uartmodet;
void uart_Thread();
void uart_init();
void UartSetMode(uartmodet mode);
uartmodet UartGetMode();
uint16_t UartGetChannel(int chno);
void UartSetChannel(int channel, const uint16_t value);
bool UartGetConnected();
const char *UartGetAddress();
int8_t UartGetRSSI();