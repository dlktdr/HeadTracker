#pragma once

#define JOYSTICK_BUTTON_HIGH 1750
#define JOYSTICK_BUTTON_LOW 1250

typedef struct {
  uint8_t but[2];
  uint16_t channels[8];
} hidreport_s;

extern const uint8_t hid_report_desc[56];

void buildJoystickHIDReport(hidreport_s &report, uint16_t chans[16]);
void joystick_init(void);
void set_JoystickChannels(uint16_t chans[16]);
