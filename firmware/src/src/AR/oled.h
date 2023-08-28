#ifndef OLED_HH
#define OLED_HH

#include <stdint.h>

void oled_init();
void oled_write_pixel(int16_t x, int16_t y);
void oled_write_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void oled_draw_diamond(int16_t x, int16_t y);
void oled_draw_square(int16_t x, int16_t y);
void oled_draw_triangle(int16_t x, int16_t y);
void oled_draw_x_shape(int16_t x, int16_t y);
void oled_update();
void oled_clean();

#endif /* OLED_HH */