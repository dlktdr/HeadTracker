#include <zephyr.h>

#include "io.h"
#include "lsm_common.h"
#include "defines.h"

int32_t platform_read_lsm6(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
  const struct device *i2c_dev = device_get_binding("I2C_1");
  return i2c_burst_read(i2c_dev, 0x6A, reg, bufp, len);
}

int32_t platform_write_lsm6(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
  const struct device *i2c_dev = device_get_binding("I2C_1");
  return i2c_burst_write(i2c_dev, 0x6A, reg, bufp, len);
}

int initailizeLSM6DS3(stmdev_ctx_t *dev_ctx)
{
  // Power up the LSM6DS3 on the XIAO Sense
  #if defined(PCB_XIAOSENSE)
  pinMode(IO_LSM6DS3PWR, GPIO_OUTPUT);
  digitalWrite(IO_LSM6DS3PWR, 1);
  #endif
  uint8_t whoamI=0, rst;
  /* Initialize mems driver interface */
  dev_ctx->write_reg = platform_write_lsm6;
  dev_ctx->read_reg = platform_read_lsm6;
  /* Wait sensor boot time */
  k_msleep(BOOT_TIME);
  /* Check device ID */
  lsm6ds3tr_c_device_id_get(dev_ctx, &whoamI);
  if (whoamI != LSM6DS3TR_C_ID)
    return -1;
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