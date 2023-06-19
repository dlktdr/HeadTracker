
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include <device.h>
#include <display/cfb.h>
#include <drivers/display.h>
#include <kernel.h>

#include "oled.h"
#include "log.h"

#define D_S 6 // DIAMOND_SIZE

static const struct device *display = DEVICE_DT_GET(DT_NODELABEL(ssd1306));

int16_t WIDTH = 128;        ///< This is the 'raw' display width - never changes
int16_t HEIGHT = 64;       ///< This is the 'raw' display height - never changes

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

uint8_t buf_clear[1024] = {0};

void clear_display()
{
  for(uint16_t i = 0; i < 1024; i++) {
    buf_clear[i] = 0;
  }
}

#define ADJUST_PAN -800

uint8_t reverse(uint8_t num)
{
    unsigned int NO_OF_BITS = 8;
    unsigned int reverse_num = 0;
    int i;
    for (i = 0; i < NO_OF_BITS; i++) {
        if ((num & (1 << i)))
            reverse_num |= 1 << ((NO_OF_BITS - 1) - i);
    }
    return reverse_num;
}

void writePixel(int16_t x, int16_t y, uint16_t color) {
  if (buf_clear) {
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

    uint8_t *ptr = &buf_clear[((x % 128) + (y / 8) * 128)];
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

void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
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
      writePixel(y0, x0, color);
    } else {
      writePixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

int16_t tilt;
int16_t roll;
int16_t pan;

void DrawN()
{
  // line
  writeLine(ADJUST_PAN+70-pan, -50+roll+10, ADJUST_PAN+70-pan, -50+roll+20, 1);
  // N Char
  writeLine(ADJUST_PAN+70-pan-2, -50+roll+8, ADJUST_PAN+70-pan-2, -50+roll+2, 1);
  writeLine(ADJUST_PAN+70-pan+2, -50+roll+8, ADJUST_PAN+70-pan-2, -50+roll+2, 1);
  writeLine(ADJUST_PAN+70-pan+2, -50+roll+8, ADJUST_PAN+70-pan+2, -50+roll+2, 1);
}

void drawPosition(int16_t x, int y)
{
  writeLine(x - D_S, y      , x      , y + D_S, 1);
  writeLine(x      , y + D_S, x + D_S, y      , 1);
  writeLine(x + D_S, y      , x      , y - D_S, 1);
  writeLine(x      , y - D_S, x - D_S, y      , 1);
}

#define CALIBRATION_MULTIPLY 7

void oled_set_tilt(float tilt_new)
{
  tilt = (int16_t)(tilt_new * CALIBRATION_MULTIPLY);
}
void oled_set_roll(float roll_new)
{
  roll = (int16_t)(-roll_new * CALIBRATION_MULTIPLY);
}
void oled_set_pan(float pan_new)
{
  pan = (int16_t)(-pan_new * CALIBRATION_MULTIPLY);
}

int16_t temp_pan = -10000;
int16_t temp_roll = -10000;

void set_oled_pan_roll(uint32_t pan_new, uint32_t roll_new)
{
  temp_pan = (int16_t)(-((int16_t)pan_new) * CALIBRATION_MULTIPLY);
  temp_roll = (int16_t)(-((int16_t)roll_new) * CALIBRATION_MULTIPLY);
  LOGI("temp_pan = %d, temp_roll, %d", temp_pan, temp_roll);
  // LOGI("pan = %d, roll, %d", temp_pan, temp_roll);
}

void oled_Thread()
{
/* waiting until log functions initialize */
  rt_sleep_ms(3000);

  LOGI("Oled thread started");

  if (display == NULL) {
    LOGI("device pointer is NULL");
  } else {
    LOGI("device pointer is OK");
  }

  if (!device_is_ready(display)) {
    LOGI("display device is not ready");
    return;
  } else {
    LOGI("device is ready");
  }

  const struct display_buffer_descriptor buf_desc = {
    .buf_size = 128 * 64,
    .width = 128,
    .height = 64,
    .pitch = 128
  };

  if (display_write(display, 0, 0, &buf_desc, buf_clear) != 0) {
    LOGI("could not write to display");
  } else {
    LOGI("Written to display");
  }

  if (display_set_contrast(display, 128) != 0) {
    LOGI("could not set display contrast");
  } else {
    LOGI("Contrast set");
  }

  writeLine(0,0,127,63,1);
  writeLine(0,63,127,0,1);
  writeLine(0,0,127,0,1);
  writeLine(0,0,0,63,1);
  writeLine(127,0,127,63,1);
  writeLine(0,63,127,63,1);
  display_write(display, 0, 0, &buf_desc, buf_clear);

  rt_sleep_ms(1500);

  display_write(display, 0, 0, &buf_desc, buf_clear);

  drawPosition(30, 30);

  while (1) {
    clear_display();
    DrawN();
    drawPosition(ADJUST_PAN-pan, roll+30);
    drawPosition(ADJUST_PAN-pan+50, roll+40);
    drawPosition(ADJUST_PAN-pan-50, roll+60);
    drawPosition(ADJUST_PAN-pan-temp_pan, roll+temp_roll);
    display_write(display, 0, 0, &buf_desc, buf_clear);
    rt_sleep_ms(25);
  }
}