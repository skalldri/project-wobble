#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include "imu.h"

#define I2C_DEV CONFIG_I2C_0_NAME
#define CONFIG_MPU9250_NAME "mpu9250"

void imu_task(void *arg1, void *arg2, void *arg3)
{
    struct device* i2c_dev;
	struct sensor_value sensor_values[3] = {0};

	printf("\r\n");

	i2c_dev = device_get_binding(CONFIG_MPU9250_NAME); //pointer to a device
	if (!i2c_dev) {
		printf("I2C: Device driver not found.\r\n");
		return;
	}

	while(true) {
		sensor_sample_fetch(i2c_dev); // gets next sample from driver
		// sensor_channel_get(i2c_dev, SENSOR_CHAN_ACCEL_XYZ, sensor_values);
		// printf("accel x: %e\n", sensor_value_to_double(&sensor_values[0]));
		// printf("accel y: %e\n", sensor_value_to_double(&sensor_values[1]));
		// printf("accel z: %e", sensor_value_to_double(&sensor_values[2]));

		sensor_channel_get(i2c_dev, SENSOR_CHAN_GYRO_XYZ, sensor_values);
		printf("gyro x: %e\n", sensor_value_to_double(&sensor_values[0]));
		printf("gyro y: %e\n", sensor_value_to_double(&sensor_values[1]));
		printf("gyro z: %e", sensor_value_to_double(&sensor_values[2]));
	}
}