#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include "ui.h"

#define LCD_DEV "lcd1602"
#define OLED_DEV "ssd1306"

void ui_task(void *arg1, void *arg2, void *arg3)
{
    struct device* oled_dev;

	printf("\r\n");

    oled_dev = device_get_binding(OLED_DEV);
    if (!oled_dev) {
		printf("OLED: Device driver not found.\r\n");
        return;
    }

    ssd1306_refresh(oled_dev);

    while(true)
    {
        k_sleep(1000);
    }
}