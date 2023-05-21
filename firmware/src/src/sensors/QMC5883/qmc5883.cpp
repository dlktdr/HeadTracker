/*
 * This file is part of INAV Project.
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
 *
 * Cliff Blackburn - I2C Read/Writes modified for use in zephyr
 */

#include <stdbool.h>
#include <stdint.h>

#include <math.h>

#include "qmc5883.h"

#define QMC5883L_MAG_I2C_ADDRESS     0x0D

// Registers
#define QMC5883L_REG_CONF1 0x09
#define QMC5883L_REG_CONF2 0x0A

// data output rates for 5883L
#define QMC5883L_ODR_10HZ (0x00 << 2)
#define QMC5883L_ODR_50HZ  (0x01 << 2)
#define QMC5883L_ODR_100HZ (0x02 << 2)
#define QMC5883L_ODR_200HZ (0x03 << 2)

// Sensor operation modes
#define QMC5883L_MODE_STANDBY 0x00
#define QMC5883L_MODE_CONTINUOUS 0x01

#define QMC5883L_RNG_2G (0x00 << 4)
#define QMC5883L_RNG_8G (0x01 << 4)

#define QMC5883L_OSR_512 (0x00 << 6)
#define QMC5883L_OSR_256 (0x01 << 6)
#define QMC5883L_OSR_128	(0x10 << 6)
#define QMC5883L_OSR_64	(0x11	<< 6)

#define QMC5883L_RST 0x80

#define QMC5883L_REG_DATA_OUTPUT_X 0x00
#define QMC5883L_REG_STATUS 0x06

#define QMC5883L_REG_ID 0x0D
#define QMC5883_ID_VAL 0xFF

#define I2C_DEV "I2C_1"

static bool i2c_writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data)
{
  const struct device* i2c_dev = device_get_binding(I2C_DEV);
  if (!i2c_dev) {
    LOGE("Could not get device binding for I2C");
  }
  if(length > 49) {
    LOGE("I2C: Buffer too small");
    return false;
  }

  uint8_t buffer[50];
  buffer[0] = regAddr;
  memcpy(buffer + 1, data, length);
  return i2c_write(i2c_dev, buffer, length + 1, devAddr) == 0;
}

static bool i2c_readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data)
{
  const struct device* i2c_dev = device_get_binding(I2C_DEV);
  if (!i2c_dev) {
    LOGE("Could not get device binding for I2C");
    return -1;
  }

  return i2c_write_read(i2c_dev, devAddr, &regAddr, 1, data, length) == 0;
}

bool qmc5883Init()
{
    bool ack = true;
    uint8_t data = 0x01;

    ack = ack && i2c_writeBytes(QMC5883L_MAG_I2C_ADDRESS, 0x0B, 1, &data);
    // ack = ack && i2cWrite(busWrite(mag->busDev, 0x20, 0x40);
    // ack = ack && i2cWrite(busWrite(mag->busDev, 0x21, 0x01);
    data = QMC5883L_MODE_CONTINUOUS | QMC5883L_ODR_200HZ | QMC5883L_OSR_512 | QMC5883L_RNG_2G;
    ack = ack && i2c_writeBytes(QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_CONF1, 1, &data);

    return ack;
}

bool qmc5883Read(float mag[3])
{
    uint8_t status;
    uint8_t buf[6];

    // set magData to zero for case of failed read
    mag[0] = 0;
    mag[1] = 0;
    mag[2] = 0;

    bool ack = i2c_readBytes(QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_STATUS, 1, &status);
    if (!ack || (status & 0x04) == 0) {
        return false;
    }

    ack = i2c_readBytes(QMC5883L_MAG_I2C_ADDRESS, QMC5883L_REG_DATA_OUTPUT_X, 6, buf);
    if (!ack) {
        return false;
    }

    mag[0] = (int16_t)(buf[1] << 8 | buf[0]);
    mag[1] = (int16_t)(buf[3] << 8 | buf[2]);
    mag[2] = (int16_t)(buf[5] << 8 | buf[4]);

    mag[0] /= 163.84; // 16Bit +/-2 Gauss to uT
    mag[1] /= 163.84;
    mag[2] /= 163.84;

    return true;
}
