#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include "imu.h"

#define I2C_DEV CONFIG_I2C_0_NAME

void imu_task(void *arg1, void *arg2, void *arg3)
{
    struct device* i2c_dev;

	printf("\r\n");

	i2c_dev = device_get_binding(I2C_DEV);
	if (!i2c_dev) {
		printf("I2C: Device driver not found.\r\n");
		return;
	}
}