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

#include <stdbool.h>
#include <stdint.h>

#include "position.h"
#include "log.h"
#include "oled.h"

#define CALIBRATION_MULTIPLY 5

#define POINTS_MAX 16

struct Position_Data_T {
  Point_Data_T point;
  enum Point_Type_T point_type;
};

struct Position_Data_T positions[POINTS_MAX];

int16_t pos_tilt;
int16_t pos_roll;
int16_t pos_pan;

void position_set_pitch(float tilt_new)
{
  pos_tilt = (int16_t)(tilt_new * CALIBRATION_MULTIPLY);
}
void position_set_roll(float roll_new)
{
  pos_roll = (int16_t)(-roll_new * CALIBRATION_MULTIPLY);
}
void position_set_azimuth(float pan_new)
{
  pos_pan = (int16_t)(-pan_new * CALIBRATION_MULTIPLY - 250);
}

void position_add_point(struct Point_Data_T point_data, enum Point_Type_T point_type)
{
  uint32_t i = 0;
  for (i = 0; i < POINTS_MAX; i++) {
    if (positions[i].point.name[0] == 0)
    {
      positions[i].point = point_data;
      positions[i].point_type = point_type;
      break;
    }
  }
}

void position_Thread()
{
/* waiting until log functions initialize */
  // rt_sleep_ms(3000);

  LOGI("Position thread started");

  oled_init();
  oled_clean();
  oled_draw_diamond(20, 20);
  oled_draw_square(30, 20);
  oled_draw_triangle(40, 20);
  oled_draw_x_shape(50, 20);
  oled_draw_circle(60, 20);
  oled_write_N(20, 30, 3);
  oled_write_E(30, 30, 3);
  oled_write_S(40, 30, 3);
  oled_write_W(50, 30, 3);
  oled_update();

  while (1) {

    rt_sleep_ms(25);
  }
}