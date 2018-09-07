/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Convert the 5060 driver into a 9250 driver, make it work over spi instead of i2c
 * imu.c should be in the app folder
 */

#include <i2c.h>
#include <init.h>
#include <misc/byteorder.h>
#include <sensor.h>

#include "mpu9250.h"
#include "mpu9250_config.h"

/* see "Accelerometer Measurements" section from register map description */
static void mpu9250_convert_accel(struct sensor_value *val, s16_t raw_val,
				  u16_t sensitivity_shift)
{
	s64_t conv_val;

	conv_val = ((s64_t)raw_val * SENSOR_G) >> sensitivity_shift;
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Gyroscope Measurements" section from register map description */
static void mpu9250_convert_gyro(struct sensor_value *val, s16_t raw_val,
				 u16_t sensitivity_x10)
{
	s64_t conv_val;

	conv_val = ((s64_t)raw_val * SENSOR_PI * 10) /
		   (180 * sensitivity_x10);
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Temperature Measurement" section from register map description */
static inline void mpu9250_convert_temp(struct sensor_value *val,
					s16_t raw_val)
{
	val->val1 = raw_val / 340 + 36;
	val->val2 = ((s64_t)(raw_val % 340) * 1000000) / 340 + 530000;

	if (val->val2 < 0) {
		val->val1--;
		val->val2 += 1000000;
	} else if (val->val2 >= 1000000) {
		val->val1++;
		val->val2 -= 1000000;
	}
}

//pull
static int mpu9250_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct mpu9250_data *drv_data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		mpu9250_convert_accel(val, drv_data->accel_x,
				      drv_data->accel_sensitivity_shift);
		mpu9250_convert_accel(val + 1, drv_data->accel_y,
				      drv_data->accel_sensitivity_shift);
		mpu9250_convert_accel(val + 2, drv_data->accel_z,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		mpu9250_convert_accel(val, drv_data->accel_x,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		mpu9250_convert_accel(val, drv_data->accel_y,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		mpu9250_convert_accel(val, drv_data->accel_z,
				      drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		mpu9250_convert_gyro(val, drv_data->gyro_x,
				     drv_data->gyro_sensitivity_x10);
		mpu9250_convert_gyro(val + 1, drv_data->gyro_y,
				     drv_data->gyro_sensitivity_x10);
		mpu9250_convert_gyro(val + 2, drv_data->gyro_z,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		mpu9250_convert_gyro(val, drv_data->gyro_x,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		mpu9250_convert_gyro(val, drv_data->gyro_y,
				     drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		mpu9250_convert_gyro(val, drv_data->gyro_z,
				     drv_data->gyro_sensitivity_x10);
		break;
	default: /* chan == SENSOR_CHAN_DIE_TEMP */
		mpu9250_convert_temp(val, drv_data->temp);
	}

	return 0;
}

//main updater
static int mpu9250_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct mpu9250_data *drv_data = dev->driver_data;
	s16_t buf[7];

	if (i2c_burst_read(drv_data->i2c, CONFIG_MPU9250_I2C_ADDR,
			   MPU9250_REG_DATA_START, (u8_t *)buf, 14) < 0) {
		SYS_LOG_ERR("Failed to read data sample.");
		return -EIO;
	}

	drv_data->accel_x = sys_be16_to_cpu(buf[0]);
	drv_data->accel_y = sys_be16_to_cpu(buf[1]);
	drv_data->accel_z = sys_be16_to_cpu(buf[2]);
	drv_data->temp = sys_be16_to_cpu(buf[3]);
	drv_data->gyro_x = sys_be16_to_cpu(buf[4]);
	drv_data->gyro_y = sys_be16_to_cpu(buf[5]);
	drv_data->gyro_z = sys_be16_to_cpu(buf[6]);

	return 0;
}

//push
static const struct sensor_driver_api mpu9250_driver_api = {
#if CONFIG_MPU9250_TRIGGER
	.trigger_set = mpu9250_trigger_set,
#endif
	.sample_fetch = mpu9250_sample_fetch,
	.channel_get = mpu9250_channel_get,
};

int mpu9250_init(struct device *dev)
{
	struct mpu9250_data *drv_data = dev->driver_data;
	u8_t id, i;

	drv_data->i2c = device_get_binding(CONFIG_MPU9250_I2C_MASTER_DEV_NAME); // This is the name of the bus in ninja config menu - kconfig (SPI_1)
	// we didn't find the bus to talk to the chip, fail
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device",
			    CONFIG_MPU9250_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	/* check chip ID
		if the chip is connected to the bus physically
	 */
	if (i2c_reg_read_byte(drv_data->i2c, CONFIG_MPU9250_I2C_ADDR,
			      MPU9250_REG_CHIP_ID, &id) < 0) {
		SYS_LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	// bail if wrong chip id
	if (id != MPU9250_CHIP_ID) {
		SYS_LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* wake up chip
	// reads and writes to the chip all in one function
	set the sleep_enable bit on the device (so wake up) */
	if (i2c_reg_update_byte(drv_data->i2c, CONFIG_MPU9250_I2C_ADDR,
				MPU9250_REG_PWR_MGMT1, MPU9250_SLEEP_EN,
				0) < 0) {
		SYS_LOG_ERR("Failed to wake up chip.");
		return -EIO;
	}

	/* set accelerometer full-scale range */
	for (i = 0; i < 4; i++) {
		if (BIT(i+1) == CONFIG_MPU9250_ACCEL_FS) {
			break;
		}
	}

	if (i == 4) {
		SYS_LOG_ERR("Invalid value for accel full-scale range.");
		return -EINVAL;
	}

	if (i2c_reg_write_byte(drv_data->i2c, CONFIG_MPU9250_I2C_ADDR,
			       MPU9250_REG_ACCEL_CFG,
			       i << MPU9250_ACCEL_FS_SHIFT) < 0) {
		SYS_LOG_ERR("Failed to write accel full-scale range.");
		return -EIO;
	}

	drv_data->accel_sensitivity_shift = 14 - i; //idk

	/* set gyroscope full-scale range */
	for (i = 0; i < 4; i++) {
		if (BIT(i) * 250 == CONFIG_MPU9250_GYRO_FS) {
			break;
		}
	}

	if (i == 4) {
		SYS_LOG_ERR("Invalid value for gyro full-scale range.");
		return -EINVAL;
	}

	if (i2c_reg_write_byte(drv_data->i2c, CONFIG_MPU9250_I2C_ADDR,
			       MPU9250_REG_GYRO_CFG,
			       i << MPU9250_GYRO_FS_SHIFT) < 0) {
		SYS_LOG_ERR("Failed to write gyro full-scale range.");
		return -EIO;
	}

	drv_data->gyro_sensitivity_x10 = mpu9250_gyro_sensitivity_x10[i];

#ifdef CONFIG_MPU9250_TRIGGER //important
	if (mpu9250_init_interrupt(dev) < 0) {
		SYS_LOG_DBG("Failed to initialize interrupts.");
		return -EIO;
	}
#endif

	return 0;
}

struct mpu9250_data mpu9250_driver;


// CONFIG_MPU9250_NAME
// name of driver/chip
// this is what we want to search for in imu 
DEVICE_AND_API_INIT(mpu9250, CONFIG_MPU9250_NAME, mpu9250_init, &mpu9250_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &mpu9250_driver_api);
