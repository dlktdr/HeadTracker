#ifndef NAVIGATION_HH
#define NAVIGATION_HH

struct NAV_POINT {
    char name[16];
    float azimuth;
    float pitch;
    uint32_t distance;
};

void navigation_Thread();
void navigation_set_azimuth(float azimuth_new);
void navigation_set_pitch(float pitch_new);
void navigation_set_roll(float roll_new);
void navigation_add_point(struct NAV_POINT);
void navigation_delete_point(char name[16]);

#endif /* NAVIGATION_HH */