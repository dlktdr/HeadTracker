#ifndef OLED_HH
#define OLED_HH

#include <stdint.h>

#define X_CENTER    63
#define Y_CENTER    31

enum Line_T {
  Solid = 0,
  Cropped_0,
  Cropped_1,
  Dashed_0,
  Dashed_1
};

void oled_write_char(int16_t x, int16_t y, char letter, uint8_t font_size);
void oled_write_text(int16_t x, int16_t y, char* text, uint8_t text_size, bool center);

void oled_init(uint32_t delay);
void oled_write_pixel(int16_t x, int16_t y);
void oled_write_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, enum Line_T line_type);
void oled_draw_diamond(int16_t x, int16_t y, bool cropped);
void oled_draw_square(int16_t x, int16_t y);
void oled_draw_triangle(int16_t x, int16_t y);
void oled_draw_x_shape(int16_t x, int16_t y);
void oled_draw_circle(int16_t x0, int16_t y0);
void oled_update();
void oled_clean();

#endif /* OLED_HH */