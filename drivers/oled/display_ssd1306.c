#include <i2c.h>
#include <init.h>

#include "display_ssd1306.h"

// Write a command (1 byte of data) to the display controller
int ssd1306_send_command(struct device *dev, uint8_t data)
{
    struct ssd1306_data *drv_data = dev->driver_data;
    uint8_t command[2];
    command[0] = 0; // No Co or DC bit: single payload byte, interpreted as a command
    command[1] = data;
    return i2c_write(drv_data->i2c, &data, sizeof(command), CONFIG_SSD1306_I2C_ADDR);
}

int ssd1306_init(struct device *dev)
{
    struct ssd1306_data *drv_data = dev->driver_data;

    if (CONFIG_SSD1306_OLED_ROWS == 64)
    {
        drv_data->rows = DISPLAY_ROWS_64;
    }
    else if (CONFIG_SSD1306_OLED_ROWS == 32)
    {
        drv_data->rows = DISPLAY_ROWS_32;
    }
    else
    {
        __ASSERT(FALSE, "SSD1306_OLED_ROWS must be 32 or 64");
    }

    drv_data->i2c = device_get_binding(CONFIG_SSD1306_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device",
			    CONFIG_SSD1306_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

    // Start powering up the display. Run through the init sequence
    ssd1306_send_command(drv_data->i2c, SSD1306_DISPLAYOFF);
    ssd1306_send_command(drv_data->i2c, SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_send_command(drv_data->i2c, 0x80);
    ssd1306_send_command(drv_data->i2c, SSD1306_SETMULTIPLEX);
    ssd1306_send_command(drv_data->i2c, 0x3F);
    ssd1306_send_command(drv_data->i2c, SSD1306_SETDISPLAYOFFSET);
    ssd1306_send_command(drv_data->i2c, 0x0);
    ssd1306_send_command(drv_data->i2c, SSD1306_SETSTARTLINE | 0x0);
    ssd1306_send_command(drv_data->i2c, SSD1306_CHARGEPUMP);
    ssd1306_send_command(drv_data->i2c, 0x14); // Turn on the internal charge pump to generate the OLED drive voltage

    ssd1306_send_command(drv_data->i2c, SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_send_command(drv_data->i2c, 0x00);                                  // 0x0 act like ks0108

    ssd1306_send_command(drv_data->i2c, SSD1306_SEGREMAP | 0x1);
    ssd1306_send_command(drv_data->i2c, SSD1306_COMSCANDEC);
    ssd1306_send_command(drv_data->i2c, SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_send_command(drv_data->i2c, 0x12);

    ssd1306_send_command(drv_data->i2c, SSD1306_SETCONTRAST);                   // 0x81
    ssd1306_send_command(drv_data->i2c, 0xCF);

    ssd1306_send_command(drv_data->i2c, SSD1306_SETPRECHARGE);                  // 0xd9
    ssd1306_send_command(drv_data->i2c, 0xF1);

    ssd1306_send_command(drv_data->i2c, SSD1306_SETVCOMDETECT);                 // 0xDB
    ssd1306_send_command(drv_data->i2c, 0x40);

    ssd1306_send_command(drv_data->i2c, SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_send_command(drv_data->i2c, SSD1306_NORMALDISPLAY);                 // 0xA6
    
    ssd1306_send_command(drv_data->i2c, 0xb0);
    ssd1306_send_command(drv_data->i2c, 0x10);
    ssd1306_send_command(drv_data->i2c, 0x01);//Set original position to (0,0)

    ssd1306_send_command(drv_data->i2c, SSD1306_DISPLAYON); //--turn on oled panel

    /* device_get_binding checks if driver_api is not zero before checking
	 * device name.
	 * So just set driver_api to 1 else the function call will fail
     * TODO: add a read Character LCD interface
	 */
	dev->driver_api = (void *)1;

    return 0;
}

struct ssd1306_data ssd1306_driver;

DEVICE_INIT(ssd1306, CONFIG_SSD1306_NAME, ssd1306_init, &ssd1306_driver,
		    NULL, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);