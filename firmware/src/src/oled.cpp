#include "oled.h"
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include <zephyr.h>

#include "drivers/spi.h"


void oled_Thread()
{
/* waiting until log functions initialize */
  rt_sleep_ms(3000);

  LOGI("Oled thread started");
  while (1) {
    rt_sleep_ms(500);
  }
}