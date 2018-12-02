#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include "imu.h"
#include "drv8825.h"

#define I2C_DEV CONFIG_I2C_0_NAME
#define CONFIG_MPU9250_NAME "mpu9250"
#define CONFIG_DRV8825_0_NAME "drv8825_0"
#define CONFIG_DRV8825_1_NAME "drv8825_1"

void imu_task(void *arg1, void *arg2, void *arg3)
{
    struct device* i2c_dev;
	struct device* motor_0_dev;
	struct device* motor_1_dev;
	struct sensor_value sensor_values[3] = {0};

	printf("\r\n");

	i2c_dev = device_get_binding(CONFIG_MPU9250_NAME); //pointer to a device
	if (!i2c_dev) {
		printf("ERROR: IMU Device driver not found.\r\n");
		return;
	}

	motor_0_dev = device_get_binding(CONFIG_DRV8825_0_NAME); //pointer to a device
	if (!motor_0_dev) {
		printf("ERROR: Motor Driver 0 Device driver not found.\r\n");
		return;
	}

	motor_1_dev = device_get_binding(CONFIG_DRV8825_1_NAME); //pointer to a device
	if (!motor_0_dev) {
		printf("ERROR: Motor Driver 1 Device driver not found.\r\n");
		return;
	}

	drv8825_set_pulse_per_second(motor_0_dev, 120);
	drv8825_start(motor_0_dev);

	/*drv8825_set_pulse_per_second(motor_1_dev, 120);
	drv8825_start(motor_1_dev);*/

	while(true) 
	{
		sensor_sample_fetch(i2c_dev); // gets next sample from driver

		sensor_channel_get(i2c_dev, SENSOR_CHAN_ACCEL_XYZ, sensor_values);
		/*printf("accel x: %e\r\n", sensor_value_to_double(&sensor_values[0]));
		printf("accel y: %e\r\n", sensor_value_to_double(&sensor_values[1]));
		printf("accel z: %e\r\n", sensor_value_to_double(&sensor_values[2]));*/

		sensor_channel_get(i2c_dev, SENSOR_CHAN_GYRO_XYZ, sensor_values);
		printf("gyro x: %e\r\n", sensor_value_to_double(&sensor_values[0])); // X is the balancing channel

		//printf("gyro y: %e\r\n", sensor_value_to_double(&sensor_values[1]));
		//printf("gyro z: %e\r\n", sensor_value_to_double(&sensor_values[2]));

		k_sleep(10);
	}
}