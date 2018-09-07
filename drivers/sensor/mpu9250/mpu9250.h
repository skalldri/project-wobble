/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_MPU9250_H__
#define __SENSOR_MPU9250_H__

#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <zephyr/types.h>

#define SYS_LOG_DOMAIN "MPU9250"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>

#define MPU9250_REG_CHIP_ID		0x75
#define MPU9250_CHIP_ID			115

#define MPU9250_REG_GYRO_CFG		0x1B
#define MPU9250_GYRO_FS_SHIFT		3

#define MPU9250_REG_ACCEL_CFG		0x1C
#define MPU9250_ACCEL_FS_SHIFT		3

#define MPU9250_REG_INT_EN		0x38
#define MPU9250_DRDY_EN			BIT(0)

#define MPU9250_REG_DATA_START		0x3B

#define MPU9250_REG_PWR_MGMT1		0x6B
#define MPU9250_SLEEP_EN		BIT(6)

/* measured in degrees/sec x10 to avoid floating point */
static const u16_t mpu9250_gyro_sensitivity_x10[] = {
	1310, 655, 328, 164
};

struct mpu9250_data {
	struct device *i2c;

	s16_t accel_x;
	s16_t accel_y;
	s16_t accel_z;
	u16_t accel_sensitivity_shift;

	s16_t temp;

	s16_t gyro_x;
	s16_t gyro_y;
	s16_t gyro_z;
	u16_t gyro_sensitivity_x10;

#ifdef CONFIG_MPU9250_TRIGGER
	struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

#if defined(CONFIG_MPU9250_TRIGGER_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_MPU9250_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_MPU9250_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
	struct device *dev;
#endif

#endif /* CONFIG_MPU9250_TRIGGER */
};

#ifdef CONFIG_MPU9250_TRIGGER
int mpu9250_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);

int mpu9250_init_interrupt(struct device *dev);
#endif

#endif /* __SENSOR_MPU9250__ */
