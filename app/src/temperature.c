#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include "temperature.h"

#define TEMP_DEV "TEMP_0"

void temperature_task(void *arg1, void *arg2, void *arg3)
{
    struct device* temp_sensor;

	temp_sensor = device_get_binding(TEMP_DEV);
	if (!temp_sensor) {
		printf("Temperature: no temp device\r\n");
		return;
	}

	printf("temp device is %p, name is %s\r\n",
	       temp_sensor, temp_sensor->config->name);

    while (1) 
	{
		int r;
		struct sensor_value temp_value;

		r = sensor_sample_fetch(temp_sensor);
		if (r) 
		{
			printf("sensor_sample_fetch failed return: %d\r\n", r);
			break;
		}

		r = sensor_channel_get(temp_sensor, SENSOR_CHAN_DIE_TEMP, &temp_value);
		if (r) 
		{
			printf("sensor_channel_get failed return: %d\r\n", r);
			break;
		}

		printf("Temperature is %gC\r\n", sensor_value_to_double(&temp_value));

		k_sleep(1000);
	}
}