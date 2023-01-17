/**\
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

/******************************************************************************/
#include "bmm150_common.h"


#include <stdio.h>
#include "drivers/i2c.h"
#include "bmm150.h"
#include "log.h"


/******************************************************************************/
/*!                Static variable definition                                 */

/*! Variable that holds the I2C device address or SPI chip selection */
static uint8_t dev_addr;

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * @brief Function for initialization of I2C bus.
 */
int8_t bmm150_user_i2c_init(void)
{
  /* Implement I2C bus initialization according to the target machine. */
  return 0;
}

/*!
 * @brief Function for initialization of SPI bus.
 */
int8_t bmm150_user_spi_init(void)
{
  /* Implement SPI bus initialization according to the target machine. */
  return 0;
}

/*!
 * @brief This function provides the delay for required time (Microseconds) as per the input
 * provided in some of the APIs.
 */
void bmm150_user_delay_us(uint32_t period_us, void *intf_ptr)
{
  k_usleep(period_us);
}

/*!
 * @brief This function is for writing the sensor's registers through I2C bus.
 */
int8_t bmm150_user_i2c_reg_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length,
                                 void *intf_ptr)
{
  const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
  if (!i2c_dev) {
    return -1;
  }

  uint8_t dev_addr = *(uint8_t *)intf_ptr;
  return i2c_burst_write(i2c_dev, dev_addr, reg_addr, reg_data, length);
}

/*!
 * @brief This function is for reading the sensor's registers through I2C bus.
 */
int8_t bmm150_user_i2c_reg_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length,
                                void *intf_ptr)
{
  const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
  if (!i2c_dev) {
    return -1;
  }

  uint8_t dev_addr = *(uint8_t *)intf_ptr;
  return i2c_burst_read(i2c_dev, dev_addr, reg_addr, reg_data, length);
}

/*!
 * @brief This function is for writing the sensor's registers through SPI bus.
 */
int8_t bmm150_user_spi_reg_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length,
                                 void *intf_ptr)
{
  return -1;
}

/*!
 * @brief This function is for reading the sensor's registers through SPI bus.
 */
int8_t bmm150_user_spi_reg_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t length,
                                void *intf_ptr)
{
  /* Read from registers using SPI. Return 0 for a successful execution. */
  return -1;
}

/*!
 *  @brief This function is to select the interface between SPI and I2C.
 */
int8_t bmm150_interface_selection(struct bmm150_dev *dev)
{
  int8_t rslt = BMM150_OK;

  if (dev != NULL) {
    /* Select the interface for execution
     * For I2C : BMM150_I2C_INTF
     * For SPI : BMM150_SPI_INTF
     */
    dev->intf = BMM150_I2C_INTF;

    /* Bus configuration : I2C */
    if (dev->intf == BMM150_I2C_INTF) {
      printk("I2C Interface \n");

      /* To initialize the user I2C function */
      bmm150_user_i2c_init();

      dev_addr = BMM150_DEFAULT_I2C_ADDRESS;
      dev->read = bmm150_user_i2c_reg_read;
      dev->write = bmm150_user_i2c_reg_write;
    }
    /* Bus configuration : SPI */
    else if (dev->intf == BMM150_SPI_INTF) {
      printk("SPI Interface \n");

      /* To initialize the user SPI function */
      bmm150_user_spi_init();

      dev_addr = 0;
      dev->read = bmm150_user_spi_reg_read;
      dev->write = bmm150_user_spi_reg_write;
    }

    /* Assign device address to interface pointer */
    dev->intf_ptr = &dev_addr;

    /* Configure delay in microseconds */
    dev->delay_us = bmm150_user_delay_us;
  } else {
    rslt = BMM150_E_NULL_PTR;
  }

  return rslt;
}

/*!
 * @brief This internal API prints the execution status
 */
void bmm150_error_codes_print_result(const char api_name[], int8_t rslt)
{
  if (rslt != BMM150_OK) {
    printk("%s\t", api_name);

    switch (rslt) {
      case BMM150_E_NULL_PTR:
        printk("Error [%d] : Null pointer error.", rslt);
        printk(
            "It occurs when the user tries to assign value (not address) to a pointer, which has "
            "been initialized to NULL.\r\n");
        break;

      case BMM150_E_COM_FAIL:
        printk("Error [%d] : Communication failure error.", rslt);
        printk(
            "It occurs due to read/write operation failure and also due to power failure during "
            "communication\r\n");
        break;

      case BMM150_E_DEV_NOT_FOUND:
        printk(
            "Error [%d] : Device not found error. It occurs when the device chip id is incorrectly "
            "read\r\n",
            rslt);
        break;

      case BMM150_E_INVALID_CONFIG:
        printk("Error [%d] : Invalid sensor configuration.", rslt);
        printk(
            " It occurs when there is a mismatch in the requested feature with the available "
            "one\r\n");
        break;

      default:
        printk("Error [%d] : Unknown error code\r\n", rslt);
        break;
    }
  }
}

int8_t set_config(struct bmm150_dev *dev)
{
    /* Status of api are returned to this variable. */
    int8_t rslt;

    struct bmm150_settings settings;

    /* Set powermode as normal mode */
    settings.pwr_mode = BMM150_POWERMODE_NORMAL;
    rslt = bmm150_set_op_mode(&settings, dev);
    bmm150_error_codes_print_result("bmm150_set_op_mode", rslt);

    if (rslt == BMM150_OK)
    {
        settings.preset_mode = BMM150_PRESETMODE_LOWPOWER;
        rslt = bmm150_set_presetmode(&settings, dev);
        bmm150_error_codes_print_result("bmm150_set_presetmode", rslt);

        if (rslt == BMM150_OK)
        {
            /* Map the data interrupt pin */
            settings.int_settings.drdy_pin_en = 0x01;
            rslt = bmm150_set_sensor_settings(BMM150_SEL_DRDY_PIN_EN, &settings, dev);
            bmm150_error_codes_print_result("bmm150_set_sensor_settings", rslt);
        }
    }

    return rslt;
}