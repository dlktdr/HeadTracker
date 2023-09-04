#ifndef OLED_HH
#define OLED_HH

#include <stdint.h>

#define X_CENTER    63
#define Y_CENTER    31

void oled_init();
void oled_write_pixel(int16_t x, int16_t y);
void oled_write_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void oled_draw_diamond(int16_t x, int16_t y);
void oled_draw_square(int16_t x, int16_t y);
void oled_draw_triangle(int16_t x, int16_t y);
void oled_draw_x_shape(int16_t x, int16_t y);
void oled_draw_circle(int16_t x0, int16_t y0);
void oled_write_N(int16_t x, int16_t y, uint8_t size);
void oled_write_E(int16_t x, int16_t y, uint8_t size);
void oled_write_S(int16_t x, int16_t y, uint8_t size);
void oled_write_W(int16_t x, int16_t y, uint8_t size);
void oled_update();
void oled_clean();

#endif /* OLED_HH */