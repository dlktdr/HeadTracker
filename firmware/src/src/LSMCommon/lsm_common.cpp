#include <zephyr/kernel.h>

#include "defines.h"
#include "io.h"


int32_t platform_read_lsm6(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
  const struct device *i2c_dev = DEVICE_DT_GET(DT_ALIAS(i2csensor));
  return i2c_burst_read(i2c_dev, LSM6DS3TR_C_ID, reg, bufp, len);
}

int32_t platform_write_lsm6(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
  const struct device *i2c_dev = DEVICE_DT_GET(DT_ALIAS(i2csensor));
  return i2c_burst_write(i2c_dev, LSM6DS3TR_C_ID, reg, bufp, len);
}

int initailizeLSM6DS3(stmdev_ctx_t *dev_ctx)
{
  uint8_t whoamI = 0, rst;
  /* Initialize mems driver interface */
  dev_ctx->write_reg = platform_write_lsm;
  dev_ctx->read_reg = platform_read_lsm;
  /* Wait sensor boot time */
  k_msleep(BOOT_TIME);
  /* Check device ID */
  lsm6ds3tr_c_device_id_get(dev_ctx, &whoamI);
  if (whoamI != LSM6DS3TR_C_ID) return -1;
  /* Restore default configuration */
  lsm6ds3tr_c_reset_set(dev_ctx, PROPERTY_ENABLE);
  do {
    lsm6ds3tr_c_reset_get(dev_ctx, &rst);
  } while (rst);
  /*  Enable Block Data Update */
  lsm6ds3tr_c_block_data_update_set(dev_ctx, PROPERTY_ENABLE);
  /* Set full scale */
  lsm6ds3tr_c_xl_full_scale_set(dev_ctx, LSM6DS3TR_C_2g);
  lsm6ds3tr_c_gy_full_scale_set(dev_ctx, LSM6DS3TR_C_2000dps);
  /* Set Output Data Rate for Acc and Gyro */
  lsm6ds3tr_c_xl_data_rate_set(dev_ctx, LSM6DS3TR_C_XL_ODR_208Hz);
  lsm6ds3tr_c_gy_data_rate_set(dev_ctx, LSM6DS3TR_C_GY_ODR_208Hz);

  return 0;
}

int initailizeLSM9DS1(stmdev_ctx_t *dev_ctx_imu, stmdev_ctx_t *dev_ctx_mag)
{
  lsm9ds1_id_t whoamI;
  uint8_t rst;
  /* Initialize mems driver interface */
  dev_ctx_imu->write_reg = platform_write_lsm;
  dev_ctx_imu->read_reg = platform_read_lsm;
  /* Initialize mems driver interface */
  dev_ctx_mag->write_reg = platform_write_lsm;
  dev_ctx_mag->read_reg = platform_read_lsm;
  /* Wait sensor boot time */
  k_msleep(BOOT_TIME);
  /* Check device ID */
  lsm9ds1_dev_id_get(dev_ctx_mag, dev_ctx_imu, &whoamI);
  if (whoamI.imu != LSM9DS1_IMU_ID || whoamI.mag != LSM9DS1_MAG_ID) return -1;
  /* Restore default configuration */
  lsm9ds1_dev_reset_set(dev_ctx_mag, dev_ctx_imu, PROPERTY_ENABLE);
  do {
    lsm9ds1_dev_reset_get(dev_ctx_mag, dev_ctx_imu, &rst);
  } while (rst);
  /*  Enable Block Data Update */
  lsm9ds1_block_data_update_set(dev_ctx_mag, dev_ctx_imu, PROPERTY_ENABLE);
  /* Set full scale */
  lsm9ds1_xl_full_scale_set(dev_ctx_imu, LSM9DS1_4g);
  lsm9ds1_gy_full_scale_set(dev_ctx_imu, LSM9DS1_2000dps);
  lsm9ds1_mag_full_scale_set(dev_ctx_mag, LSM9DS1_4Ga);
  /* Configure filtering chain - See datasheet for filtering chain details */
  /* Accelerometer filtering chain */
  lsm9ds1_xl_filter_aalias_bandwidth_set(dev_ctx_imu, LSM9DS1_AUTO);
  lsm9ds1_xl_filter_lp_bandwidth_set(dev_ctx_imu, LSM9DS1_LP_ODR_DIV_50);
  lsm9ds1_xl_filter_out_path_set(dev_ctx_imu, LSM9DS1_LP_OUT);
  /* Gyroscope filtering chain */
  lsm9ds1_gy_filter_lp_bandwidth_set(dev_ctx_imu, LSM9DS1_LP_ULTRA_LIGHT);
  lsm9ds1_gy_filter_hp_bandwidth_set(dev_ctx_imu, LSM9DS1_HP_MEDIUM);
  lsm9ds1_gy_filter_out_path_set(dev_ctx_imu, LSM9DS1_LPF1_HPF_LPF2_OUT);
  /* Set Output Data Rate / Power mode */
  lsm9ds1_imu_data_rate_set(dev_ctx_imu, LSM9DS1_IMU_238Hz);
  lsm9ds1_mag_data_rate_set(dev_ctx_mag, LSM9DS1_MAG_UHP_155Hz);

  return 0;
}