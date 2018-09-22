#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include "ui.h"

#define LCD_DEV "lcd1602"

void ui_task(void *arg1, void *arg2, void *arg3)
{
    struct device* lcd_dev;

	printf("\r\n");

	lcd_dev = device_get_binding(LCD_DEV);
	if (!lcd_dev) {
		printf("LCD: Device driver not found.\r\n");
		return;
	}

    lcd1602_write(lcd_dev, 'H');
    lcd1602_write(lcd_dev, 'E');
    lcd1602_write(lcd_dev, 'L');
    lcd1602_write(lcd_dev, 'L');
    lcd1602_write(lcd_dev, 'O');
    lcd1602_write(lcd_dev, ' ');
    lcd1602_write(lcd_dev, 'W');
    lcd1602_write(lcd_dev, 'O');
    lcd1602_write(lcd_dev, 'R');
    lcd1602_write(lcd_dev, 'L');
    lcd1602_write(lcd_dev, 'D');

    lcd1602_set_backlight(lcd_dev, true);
}