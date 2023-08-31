/*
 * This file is part of AR project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 3, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include <device.h>
#include <display/cfb.h>
#include <drivers/display.h>
#include <kernel.h>

#include "oled.h"
#include "log.h"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

#define POINT_SIZE 6
#define POINT_HALF_SIZE ((int16_t) POINT_SIZE / 2)

uint8_t oled_buf[1024] = {0};

const int16_t WIDTH = 128;        ///< This is the 'raw' oled width - never changes
const int16_t HEIGHT = 64;       ///< This is the 'raw' oled height - never changes

static const struct device *oled = DEVICE_DT_GET(DT_NODELABEL(ssd1306));
uint16_t color = 1;

const struct display_buffer_descriptor buf_desc = {
    .buf_size = 128 * 64,
    .width = 128,
    .height = 64,
    .pitch = 128
};

uint8_t buf[1024] = {0};

// static void display_update();

// static int display_update()
// {
//   struct display_driver_api *api = (struct display_driver_api *)display->api;

// 	return api->write(display, 0, 0, buf_desc, buf);
// }

// static inline int display_set_contrast(const struct device *dev, uint8_t contrast)
// {
// 	struct display_driver_api *api =
// 		(struct display_driver_api *)dev->api;

// 	return api->set_contrast(dev, contrast);
// }

void oled_write_pixel(int16_t x, int16_t y)
{
  if (oled_buf) {
    if ((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT))
      return;

    uint8_t rotation = 0;

    int16_t t;
    switch (rotation) {
    case 1:
      t = x;
      x = WIDTH - 1 - y;
      y = t;
      break;
    case 2:
      x = WIDTH - 1 - x;
      y = HEIGHT - 1 - y;
      break;
    case 3:
      t = x;
      x = y;
      y = HEIGHT - 1 - t;
      break;
    case 4:
      break;
    }

    uint8_t *ptr = &oled_buf[((x % 128) + (y / 8) * 128)];
#ifdef __AVR__
    if (color)
      *ptr |= pgm_read_byte(&GFXsetBit[x & 7]);
    else
      *ptr &= pgm_read_byte(&GFXclrBit[x & 7]);
#else

    y = y % 8;

    if (color)
      *ptr |= 0x1 << (y & 7);
    else
      *ptr &= ~(0x1 << (y & 7));
#endif
  }
}

void display_write_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
#if defined(ESP8266)
  yield();
#endif
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      oled_write_pixel(y0, x0);
    } else {
      oled_write_pixel(x0, y0);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void oled_draw_diamond(int16_t x, int16_t y)
{
  display_write_line(x - POINT_HALF_SIZE, y                  , x                  , y + POINT_HALF_SIZE);
  display_write_line(x                  , y + POINT_HALF_SIZE, x + POINT_HALF_SIZE, y                  );
  display_write_line(x + POINT_HALF_SIZE, y                  , x                  , y - POINT_HALF_SIZE);
  display_write_line(x                  , y - POINT_HALF_SIZE, x - POINT_HALF_SIZE, y                  );
}

void oled_draw_square(int16_t x, int16_t y)
{
  display_write_line(x - POINT_HALF_SIZE, y - POINT_HALF_SIZE, x - POINT_HALF_SIZE, y + POINT_HALF_SIZE);
  display_write_line(x - POINT_HALF_SIZE, y + POINT_HALF_SIZE, x + POINT_HALF_SIZE, y + POINT_HALF_SIZE);
  display_write_line(x + POINT_HALF_SIZE, y + POINT_HALF_SIZE, x + POINT_HALF_SIZE, y - POINT_HALF_SIZE);
  display_write_line(x + POINT_HALF_SIZE, y - POINT_HALF_SIZE, x - POINT_HALF_SIZE, y - POINT_HALF_SIZE);
}

void oled_draw_triangle(int16_t x, int16_t y)
{
  display_write_line(x + POINT_HALF_SIZE, y + POINT_HALF_SIZE, x                  , y - POINT_HALF_SIZE);
  display_write_line(x                  , y - POINT_HALF_SIZE, x - POINT_HALF_SIZE, y + POINT_HALF_SIZE);
  display_write_line(x + POINT_HALF_SIZE, y + POINT_HALF_SIZE, x - POINT_HALF_SIZE, y + POINT_HALF_SIZE);
}

void oled_draw_x_shape(int16_t x, int16_t y)
{
  display_write_line(x - POINT_HALF_SIZE, y + POINT_HALF_SIZE, x + POINT_HALF_SIZE, y - POINT_HALF_SIZE);
  display_write_line(x - POINT_HALF_SIZE, y - POINT_HALF_SIZE, x + POINT_HALF_SIZE, y + POINT_HALF_SIZE);
}

void oled_draw_circle(int16_t x0, int16_t y0)
 {
  int16_t r = POINT_HALF_SIZE;
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  oled_write_pixel(x0, y0 + r);
  oled_write_pixel(x0, y0 - r);
  oled_write_pixel(x0 + r, y0);
  oled_write_pixel(x0 - r, y0);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    oled_write_pixel(x0 + x, y0 + y);
    oled_write_pixel(x0 - x, y0 + y);
    oled_write_pixel(x0 + x, y0 - y);
    oled_write_pixel(x0 - x, y0 - y);
    oled_write_pixel(x0 + y, y0 + x);
    oled_write_pixel(x0 - y, y0 + x);
    oled_write_pixel(x0 + y, y0 - x);
    oled_write_pixel(x0 - y, y0 - x);
  }
}

void oled_update()
{
  display_write(oled, 0, 0, &buf_desc, oled_buf);
}

void oled_clean()
{
  for(uint16_t i = 0; i < 1024; i++) {
    oled_buf[i] = 0;
  }
}

void oled_init()
{
  if (oled == NULL) {
    LOGI("oled pointer is NULL");
  } else {
    LOGI("oled pointer is OK");
  }

  if (!device_is_ready(oled)) {
    LOGI("oled device is not ready");
    return;
  } else {
    LOGI("oled device is ready");
  }

  if (display_write(oled, 0, 0, &buf_desc, oled_buf) != 0) {
    LOGI("could not write to oled");
  } else {
    LOGI("Written to oled");
  }

  if (display_set_contrast(oled, 128) != 0) {
    LOGI("could not set oled contrast");
  } else {
    LOGI("Contrast set");
  }

  display_write_line(0,0,127,63);
  display_write_line(0,63,127,0);
  display_write_line(0,0,127,0);
  display_write_line(0,0,0,63);
  display_write_line(127,0,127,63);
  display_write_line(0,63,127,63);
  display_write(oled, 0, 0, &buf_desc, oled_buf);

  rt_sleep_ms(10000);
}