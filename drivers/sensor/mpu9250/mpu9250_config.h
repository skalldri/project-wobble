/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_MPU9250_CONFIG_H__
#define __SENSOR_MPU9250_CONFIG_H__

#define CONFIG_MPU9250_I2C_ADDR 0x68
#define CONFIG_MPU9250_I2C_MASTER_DEV_NAME "I2C_0"
#define CONFIG_MPU9250_ACCEL_FS 2
#define CONFIG_MPU9250_GYRO_FS 250
#define CONFIG_MPU9250_NAME "mpu9250"

#endif /* __SENSOR_MPU9250__ */
