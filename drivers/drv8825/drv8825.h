#pragma once

#include <nrf_timer.h>
#include <device.h>

typedef enum {
    DIRECTION_CLOCKWISE,
    DIRECTION_COUNTER_CLOCKWISE
} MOTOR_DIRECTION;

int drv8825_set_pulse_per_second(struct device *dev, float pulse_per_second);

int drv8825_stop(struct device *dev);

int drv8825_start(struct device *dev);

int drv8825_set_direction(struct device *dev, MOTOR_DIRECTION dir);