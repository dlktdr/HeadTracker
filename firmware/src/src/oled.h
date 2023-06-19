#ifndef OLED_HH
#define OLED_HH

void oled_Thread();
void oled_set_tilt(float tilt_new);
void oled_set_roll(float roll_new);
void oled_set_pan(float pan_new);
void set_oled_pan_roll(uint32_t pan_new, uint32_t roll_new);


#endif /* OLED_HH */