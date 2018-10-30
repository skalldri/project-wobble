#pragma once

#include <nrf_timer.h>
#include <device.h>

int drv8825_set_pulse_per_second(struct device *dev, uint32_t pulse_per_second);

int drv8825_stop(struct device *dev);

int drv8825_start(struct device *dev);