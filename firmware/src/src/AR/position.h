#ifndef POSITION_HH
#define POSITION_HH

enum Point_Type_T {
    DIAMOND,
    SQUARE,
    CIRCLE,
    TRIANGLE,
    X_SHAPE
};

struct Position_Data_T {
  float azimuth;
  float pitch;
  uint32_t distance;
  enum Point_Type_T point_type;
  char name[16];
};

void position_set_roll(float tilt_new);
void position_set_pitch(float roll_new);
void position_set_azimuth(float pan_new);
void position_add_point(struct Position_Data_T point_data);
void position_Thread();

#endif /* POSITION_HH */