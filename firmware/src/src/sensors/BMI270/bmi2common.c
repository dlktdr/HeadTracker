/**
 * Copyright (C) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "bmi2common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bmi2_defs.h"
#include "drivers/i2c.h"

/******************************************************************************/
/*!                 Macro definitions                                         */
#define BMI2XY_SHUTTLE_ID UINT16_C(0x1B8)

/*! Macro that defines read write length */
#define READ_WRITE_LEN UINT8_C(46)

/*! Earth's gravity in m/s^2 */

/*! Macros to select the sensors                   */
#define ACCEL UINT8_C(0x00)
#define GYRO UINT8_C(0x01)

/******************************************************************************/
/*!                Static variable definition                                 */

/*! Variable that holds the I2C device address or SPI chip selection */
static uint8_t dev_addr;

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * I2C read function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len,
                                    void *intf_ptr)
{
  const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
  if (!i2c_dev) {
    return -1;
  }

  uint8_t dev_addr = *(uint8_t *)intf_ptr;
  return i2c_burst_read(i2c_dev, dev_addr, reg_addr, reg_data, len);
}

/*!
 * I2C write function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len,
                                     void *intf_ptr)
{
  const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
  if (!i2c_dev) {
    return -1;
  }

  uint8_t dev_addr = *(uint8_t *)intf_ptr;
  return i2c_burst_write(i2c_dev, dev_addr, reg_addr, reg_data, len);
  // return coines_write_i2c(dev_addr, reg_addr, (uint8_t *)reg_data, (uint16_t)len);
}

/*!
 * SPI read function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len,
                                    void *intf_ptr)
{
  // uint8_t dev_addr = *(uint8_t*)intf_ptr;

  return -1;
  // return coines_read_spi(dev_addr, reg_addr, reg_data, (uint16_t)len);
}

/*!
 * SPI write function map to COINES platform
 */
BMI2_INTF_RETURN_TYPE bmi2_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len,
                                     void *intf_ptr)
{
  // uint8_t dev_addr = *(uint8_t*)intf_ptr;
  return -1;
  // return coines_write_spi(dev_addr, reg_addr, (uint8_t *)reg_data, (uint16_t)len);
}

/*!
 * Delay function map to COINES platform
 */
void bmi2_delay_us(uint32_t period, void *intf_ptr) { k_usleep(period); }

/*!
 *  @brief Function to select the interface between SPI and I2C.
 *  Also to initialize coines platform
 */
int8_t bmi2_interface_init(struct bmi2_dev *bmi, uint8_t intf)
{
  int8_t rslt = BMI2_OK;

  if (bmi != NULL) {
    /* To initialize the user I2C function */
    dev_addr = BMI2_I2C_PRIM_ADDR;
    bmi->intf = BMI2_I2C_INTF;
    bmi->read = bmi2_i2c_read;
    bmi->write = bmi2_i2c_write;

    /* Assign device address to interface pointer */
    bmi->intf_ptr = &dev_addr;

    /* Configure delay in microseconds */
    bmi->delay_us = bmi2_delay_us;

    /* Configure max read/write length (in bytes) ( Supported length depends on target machine) */
    bmi->read_write_len = READ_WRITE_LEN;

    /* Assign to NULL to load the default config file. */
    bmi->config_file_ptr = NULL;
  } else {
    rslt = BMI2_E_NULL_PTR;
  }

  return rslt;
}

/*!
 *  @brief Prints the execution status of the APIs.
 */
void bmi2_error_codes_print_result(int8_t rslt)
{
  switch (rslt) {
    case BMI2_OK:

      /* Do nothing */
      break;

    case BMI2_W_FIFO_EMPTY:
      printk("Warning [%d] : FIFO empty\r\n", rslt);
      break;
    case BMI2_W_PARTIAL_READ:
      printk("Warning [%d] : FIFO partial read\r\n", rslt);
      break;
    case BMI2_E_NULL_PTR:
      printk(
          "Error [%d] : Null pointer error. It occurs when the user tries to assign value (not "
          "address) to a pointer,"
          " which has been initialized to NULL.\r\n",
          rslt);
      break;

    case BMI2_E_COM_FAIL:
      printk(
          "Error [%d] : Communication failure error. It occurs due to read/write operation failure "
          "and also due "
          "to power failure during communication\r\n",
          rslt);
      break;

    case BMI2_E_DEV_NOT_FOUND:
      printk(
          "Error [%d] : Device not found error. It occurs when the device chip id is incorrectly "
          "read\r\n",
          rslt);
      break;

    case BMI2_E_INVALID_SENSOR:
      printk(
          "Error [%d] : Invalid sensor error. It occurs when there is a mismatch in the requested "
          "feature with the "
          "available one\r\n",
          rslt);
      break;

    case BMI2_E_SELF_TEST_FAIL:
      printk(
          "Error [%d] : Self-test failed error. It occurs when the validation of accel self-test "
          "data is "
          "not satisfied\r\n",
          rslt);
      break;

    case BMI2_E_INVALID_INT_PIN:
      printk(
          "Error [%d] : Invalid interrupt pin error. It occurs when the user tries to configure "
          "interrupt pins "
          "apart from INT1 and INT2\r\n",
          rslt);
      break;

    case BMI2_E_OUT_OF_RANGE:
      printk(
          "Error [%d] : Out of range error. It occurs when the data exceeds from filtered or "
          "unfiltered data from "
          "fifo and also when the range exceeds the maximum range for accel and gyro while "
          "performing FOC\r\n",
          rslt);
      break;

    case BMI2_E_ACC_INVALID_CFG:
      printk(
          "Error [%d] : Invalid Accel configuration error. It occurs when there is an error in "
          "accel configuration"
          " register which could be one among range, BW or filter performance in reg address "
          "0x40\r\n",
          rslt);
      break;

    case BMI2_E_GYRO_INVALID_CFG:
      printk(
          "Error [%d] : Invalid Gyro configuration error. It occurs when there is a error in gyro "
          "configuration"
          "register which could be one among range, BW or filter performance in reg address "
          "0x42\r\n",
          rslt);
      break;

    case BMI2_E_ACC_GYR_INVALID_CFG:
      printk(
          "Error [%d] : Invalid Accel-Gyro configuration error. It occurs when there is a error in "
          "accel and gyro"
          " configuration registers which could be one among range, BW or filter performance in "
          "reg address 0x40 "
          "and 0x42\r\n",
          rslt);
      break;

    case BMI2_E_CONFIG_LOAD:
      printk(
          "Error [%d] : Configuration load error. It occurs when failure observed while loading "
          "the configuration "
          "into the sensor\r\n",
          rslt);
      break;

    case BMI2_E_INVALID_PAGE:
      printk(
          "Error [%d] : Invalid page error. It occurs due to failure in writing the correct "
          "feature configuration "
          "from selected page\r\n",
          rslt);
      break;

    case BMI2_E_SET_APS_FAIL:
      printk(
          "Error [%d] : APS failure error. It occurs due to failure in write of advance power mode "
          "configuration "
          "register\r\n",
          rslt);
      break;

    case BMI2_E_AUX_INVALID_CFG:
      printk(
          "Error [%d] : Invalid AUX configuration error. It occurs when the auxiliary interface "
          "settings are not "
          "enabled properly\r\n",
          rslt);
      break;

    case BMI2_E_AUX_BUSY:
      printk(
          "Error [%d] : AUX busy error. It occurs when the auxiliary interface buses are engaged "
          "while configuring"
          " the AUX\r\n",
          rslt);
      break;

    case BMI2_E_REMAP_ERROR:
      printk(
          "Error [%d] : Remap error. It occurs due to failure in assigning the remap axes data for "
          "all the axes "
          "after change in axis position\r\n",
          rslt);
      break;

    case BMI2_E_GYR_USER_GAIN_UPD_FAIL:
      printk(
          "Error [%d] : Gyro user gain update fail error. It occurs when the reading of user gain "
          "update status "
          "fails\r\n",
          rslt);
      break;

    case BMI2_E_SELF_TEST_NOT_DONE:
      printk(
          "Error [%d] : Self-test not done error. It occurs when the self-test process is ongoing "
          "or not "
          "completed\r\n",
          rslt);
      break;

    case BMI2_E_INVALID_INPUT:
      printk("Error [%d] : Invalid input error. It occurs when the sensor input validity fails\r\n",
             rslt);
      break;

    case BMI2_E_INVALID_STATUS:
      printk(
          "Error [%d] : Invalid status error. It occurs when the feature/sensor validity fails\r\n",
          rslt);
      break;

    case BMI2_E_CRT_ERROR:
      printk("Error [%d] : CRT error. It occurs when the CRT test has failed\r\n", rslt);
      break;

    case BMI2_E_ST_ALREADY_RUNNING:
      printk(
          "Error [%d] : Self-test already running error. It occurs when the self-test is already "
          "running and "
          "another has been initiated\r\n",
          rslt);
      break;

    case BMI2_E_CRT_READY_FOR_DL_FAIL_ABORT:
      printk(
          "Error [%d] : CRT ready for download fail abort error. It occurs when download in CRT "
          "fails due to wrong "
          "address location\r\n",
          rslt);
      break;

    case BMI2_E_DL_ERROR:
      printk(
          "Error [%d] : Download error. It occurs when write length exceeds that of the maximum "
          "burst length\r\n",
          rslt);
      break;

    case BMI2_E_PRECON_ERROR:
      printk(
          "Error [%d] : Pre-conditional error. It occurs when precondition to start the feature "
          "was not "
          "completed\r\n",
          rslt);
      break;

    case BMI2_E_ABORT_ERROR:
      printk("Error [%d] : Abort error. It occurs when the device was shaken during CRT test\r\n",
             rslt);
      break;

    case BMI2_E_WRITE_CYCLE_ONGOING:
      printk(
          "Error [%d] : Write cycle ongoing error. It occurs when the write cycle is already "
          "running and another "
          "has been initiated\r\n",
          rslt);
      break;

    case BMI2_E_ST_NOT_RUNING:
      printk(
          "Error [%d] : Self-test is not running error. It occurs when self-test running is "
          "disabled while it's "
          "running\r\n",
          rslt);
      break;

    case BMI2_E_DATA_RDY_INT_FAILED:
      printk(
          "Error [%d] : Data ready interrupt error. It occurs when the sample count exceeds the "
          "FOC sample limit "
          "and data ready status is not updated\r\n",
          rslt);
      break;

    case BMI2_E_INVALID_FOC_POSITION:
      printk(
          "Error [%d] : Invalid FOC position error. It occurs when average FOC data is obtained "
          "for the wrong"
          " axes\r\n",
          rslt);
      break;

    default:
      printk("Error [%d] : Unknown error code\r\n", rslt);
      break;
  }
}

/*!
 * @brief This internal API is used to set configurations for accel and gyro.
 */
int8_t set_accel_gyro_config(struct bmi2_dev *bmi2_dev)
{
  /* Status of api are returned to this variable. */
  int8_t rslt;

  /* Structure to define accelerometer and gyro configuration. */
  struct bmi2_sens_config config[2];

  /* Configure the type of feature. */
  config[ACCEL].type = BMI2_ACCEL;
  config[GYRO].type = BMI2_GYRO;

  /* Get default configurations for the type of feature selected. */
  rslt = bmi2_get_sensor_config(config, 2, bmi2_dev);
  bmi2_error_codes_print_result(rslt);

  /* Map data ready interrupt to interrupt pin. */
  rslt = bmi2_map_data_int(BMI2_DRDY_INT, BMI2_INT1, bmi2_dev);
  bmi2_error_codes_print_result(rslt);

  if (rslt == BMI2_OK) {
    /* NOTE: The user can change the following configuration parameters according to their
     * requirement. */
    /* Set Output Data Rate */
    config[ACCEL].cfg.acc.odr = BMI2_ACC_ODR_200HZ;

    /* Gravity range of the sensor (+/- 2G, 4G, 8G, 16G). */
    config[ACCEL].cfg.acc.range = BMI2_ACC_RANGE_2G;

    /* The bandwidth parameter is used to configure the number of sensor samples that are averaged
     * if it is set to 2, then 2^(bandwidth parameter) samples
     * are averaged, resulting in 4 averaged samples.
     * Note1 : For more information, refer the datasheet.
     * Note2 : A higher number of averaged samples will result in a lower noise level of the signal,
     * but this has an adverse effect on the power consumed.
     */
    config[ACCEL].cfg.acc.bwp = BMI2_ACC_NORMAL_AVG4;

    /* Enable the filter performance mode where averaging of samples
     * will be done based on above set bandwidth and ODR.
     * There are two modes
     *  0 -> Ultra low power mode
     *  1 -> High performance mode(Default)
     * For more info refer datasheet.
     */
    config[ACCEL].cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;

    /* The user can change the following configuration parameters according to their requirement. */
    /* Set Output Data Rate */
    config[GYRO].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;

    /* Gyroscope Angular Rate Measurement Range.By default the range is 2000dps. */
    config[GYRO].cfg.gyr.range = BMI2_GYR_RANGE_2000;

    /* Gyroscope bandwidth parameters. By default the gyro bandwidth is in normal mode. */
    config[GYRO].cfg.gyr.bwp = BMI2_GYR_NORMAL_MODE;

    /* Enable/Disable the noise performance mode for precision yaw rate sensing
     * There are two modes
     *  0 -> Ultra low power mode(Default)
     *  1 -> High performance mode
     */
    config[GYRO].cfg.gyr.noise_perf = BMI2_POWER_OPT_MODE;

    /* Enable/Disable the filter performance mode where averaging of samples
     * will be done based on above set bandwidth and ODR.
     * There are two modes
     *  0 -> Ultra low power mode
     *  1 -> High performance mode(Default)
     */
    config[GYRO].cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;

    /* Set the accel and gyro configurations. */
    rslt = bmi2_set_sensor_config(config, 2, bmi2_dev);
    bmi2_error_codes_print_result(rslt);
  }

  return rslt;
}

/*!
 * @brief This function converts lsb to meter per second squared for 16 bit accelerometer at
 * range 2G, 4G, 8G or 16G.
 */
float lsb_to_mps2(int16_t val, float g_range, uint8_t bit_width)
{
  float half_scale = ((float)(1 << bit_width) / 2.0f);

  return (GRAVITY_EARTH * val * g_range) / half_scale;
}

/*!
 * @brief This function converts lsb to degree per second for 16 bit gyro at
 * range 125, 250, 500, 1000 or 2000dps.
 */
float lsb_to_dps(int16_t val, float dps, uint8_t bit_width)
{
  float half_scale = ((float)(1 << bit_width) / 2.0f);

  return (dps / ((half_scale))) * (val);
}
