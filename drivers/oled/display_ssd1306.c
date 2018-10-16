#include <i2c.h>
#include <init.h>
#include <string.h>

#include "display_ssd1306.h"
#include "config_display_ssd1306.h"

// the memory buffer for the LCD
// TODO: make this part of the driver data so multiple driver instances can exist
static uint8_t s_backBuffer[(CONFIG_SSD1306_OLED_ROWS * CONFIG_SSD1306_OLED_COLUMNS) / 8] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x80, 0x80, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xF8, 0xE0, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80,
0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0xFF,
#if (CONFIG_SSD1306_OLED_ROWS * CONFIG_SSD1306_OLED_COLUMNS > 96*16)
0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00,
0x80, 0xFF, 0xFF, 0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x8C, 0x8E, 0x84, 0x00, 0x00, 0x80, 0xF8,
0xF8, 0xF8, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xC0, 0x80,
0x00, 0xE0, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFF, 0xC7, 0x01, 0x01,
0x01, 0x01, 0x83, 0xFF, 0xFF, 0x00, 0x00, 0x7C, 0xFE, 0xC7, 0x01, 0x01, 0x01, 0x01, 0x83, 0xFF,
0xFF, 0xFF, 0x00, 0x38, 0xFE, 0xC7, 0x83, 0x01, 0x01, 0x01, 0x83, 0xC7, 0xFF, 0xFF, 0x00, 0x00,
0x01, 0xFF, 0xFF, 0x01, 0x01, 0x00, 0xFF, 0xFF, 0x07, 0x01, 0x01, 0x01, 0x00, 0x00, 0x7F, 0xFF,
0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0xFF,
0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0x0F, 0x3F, 0x7F, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xC7, 0xC7, 0x8F,
0x8F, 0x9F, 0xBF, 0xFF, 0xFF, 0xC3, 0xC0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xFC, 0xFC,
0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xF8, 0xF8, 0xF0, 0xF0, 0xE0, 0xC0, 0x00, 0x01, 0x03, 0x03, 0x03,
0x03, 0x03, 0x01, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01,
0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x03, 0x03, 0x00, 0x00,
0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x01, 0x03, 0x01, 0x00, 0x00, 0x00, 0x03,
0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
#if (CONFIG_SSD1306_OLED_ROWS == 64)
0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x1F, 0x0F,
0x87, 0xC7, 0xF7, 0xFF, 0xFF, 0x1F, 0x1F, 0x3D, 0xFC, 0xF8, 0xF8, 0xF8, 0xF8, 0x7C, 0x7D, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x3F, 0x0F, 0x07, 0x00, 0x30, 0x30, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xC0, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xC0, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x7F, 0x3F, 0x1F,
0x0F, 0x07, 0x1F, 0x7F, 0xFF, 0xFF, 0xF8, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xE0,
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0x00, 0x00,
0x00, 0xFC, 0xFE, 0xFC, 0x0C, 0x06, 0x06, 0x0E, 0xFC, 0xF8, 0x00, 0x00, 0xF0, 0xF8, 0x1C, 0x0E,
0x06, 0x06, 0x06, 0x0C, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFE, 0xFE, 0x00, 0x00, 0x00, 0x00, 0xFC,
0xFE, 0xFC, 0x00, 0x18, 0x3C, 0x7E, 0x66, 0xE6, 0xCE, 0x84, 0x00, 0x00, 0x06, 0xFF, 0xFF, 0x06,
0x06, 0xFC, 0xFE, 0xFC, 0x0C, 0x06, 0x06, 0x06, 0x00, 0x00, 0xFE, 0xFE, 0x00, 0x00, 0xC0, 0xF8,
0xFC, 0x4E, 0x46, 0x46, 0x46, 0x4E, 0x7C, 0x78, 0x40, 0x18, 0x3C, 0x76, 0xE6, 0xCE, 0xCC, 0x80,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x0F, 0x1F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F, 0x03,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00,
0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x03, 0x07, 0x0E, 0x0C,
0x18, 0x18, 0x0C, 0x06, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x01, 0x0F, 0x0E, 0x0C, 0x18, 0x0C, 0x0F,
0x07, 0x01, 0x00, 0x04, 0x0E, 0x0C, 0x18, 0x0C, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00,
0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x07,
0x07, 0x0C, 0x0C, 0x18, 0x1C, 0x0C, 0x06, 0x06, 0x00, 0x04, 0x0E, 0x0C, 0x18, 0x0C, 0x0F, 0x07,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#endif
#endif
};

// Write a command (1 byte of data) to the display controller
int ssd1306_send_command(struct device *dev, uint8_t data)
{
    struct ssd1306_data *drv_data = dev->driver_data;
    uint8_t command[2];
    command[0] = 0; // No Co or DC bit: single payload byte, interpreted as a command
    command[1] = data;
    return i2c_write(drv_data->i2c, &command[0], sizeof(command), CONFIG_SSD1306_I2C_ADDR);
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
        __ASSERT(false, "SSD1306_OLED_ROWS must be 32 or 64");
    }

    drv_data->i2c = device_get_binding(CONFIG_SSD1306_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device",
			    CONFIG_SSD1306_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

    uint32_t config = I2C_SPEED_SET(I2C_SPEED_FAST);
    i2c_configure(drv_data->i2c, config);

    int ssd1306_status = 0;

    // Start powering up the display. Run through the init sequence
    ssd1306_status = ssd1306_send_command(dev, SSD1306_DISPLAYOFF);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETDISPLAYCLOCKDIV);
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0x80);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETMULTIPLEX);
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, CONFIG_SSD1306_OLED_ROWS - 1);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETDISPLAYOFFSET);
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0x0);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETSTARTLINE | 0x0);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_CHARGEPUMP);
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0x14); // Turn on the internal charge pump to generate the OLED drive voltage
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_MEMORYMODE);                    // 0x20
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0x00);                                  // 0x0 act like ks0108
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SEGREMAP | 0x1);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_COMSCANDEC);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETCOMPINS);                    // 0xDA
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0x12);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETCONTRAST);                   // 0x81
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0xCF);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETPRECHARGE);                  // 0xd9
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0xF1);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_SETVCOMDETECT);                 // 0xDB
    if (ssd1306_status != 0) { goto Exit; }
    ssd1306_status = ssd1306_send_command(dev, 0x40);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_NORMALDISPLAY);                 // 0xA6
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_DEACTIVATE_SCROLL);
    if (ssd1306_status != 0) { goto Exit; }

    ssd1306_status = ssd1306_send_command(dev, SSD1306_DISPLAYON); //--turn on oled panel
    if (ssd1306_status != 0) { goto Exit; }

    /* device_get_binding checks if driver_api is not zero before checking
	 * device name.
	 * So just set driver_api to 1 else the function call will fail
     * TODO: add an OLED Zephyr interface
	 */
	dev->driver_api = (void *)1;

Exit:
    if (ssd1306_status != 0)
    {
        SYS_LOG_ERR("SSD1306 failed to initialize, error %d", ssd1306_status);
    }

    return ssd1306_status;
}

// Send the contents of the back buffer to the display
int ssd1306_refresh(struct device* dev)
{
    struct ssd1306_data *drv_data = dev->driver_data;
    int status = 0;

    ssd1306_send_command(dev, SSD1306_COLUMNADDR);
    ssd1306_send_command(dev, 0);   // Column start address (0 = reset)
    ssd1306_send_command(dev, CONFIG_SSD1306_OLED_COLUMNS-1); // Column end address (127 = reset)

    ssd1306_send_command(dev, SSD1306_PAGEADDR);
    ssd1306_send_command(dev, 0); // Page start address (0 = reset)

#if CONFIG_SSD1306_OLED_ROWS == 64
    ssd1306_send_command(dev, 7); // Page end address
#elif CONFIG_SSD1306_OLED_ROWS == 32
    ssd1306_send_command(dev, 3); // Page end address
#elif CONFIG_SSD1306_OLED_ROWS == 16
    ssd1306_send_command(dev, 1); // Page end address
#endif

    // I2C
    for (uint16_t i = 0; i < ((CONFIG_SSD1306_OLED_COLUMNS * CONFIG_SSD1306_OLED_ROWS) / 8); i += 16)
    {
        // Do not use i2c_burst_write for this application.
        // The Zephyr I2C API will insert a repeat-start condition between writing the register address
        // and the data blob, which confuses the SSD1306. Zephyr also does not support scatter-gather I2C 
        // data transactions.
        // Create a command buffer here to hold the full command (0x40 + 16 data bytes) before transmitting to the 
        // chip as one long i2C_write.
        uint8_t command[17];
        
        command[0] = 0x40;
        memcpy(&command[1], &s_backBuffer[i], 16);

        status = i2c_write(drv_data->i2c, command, sizeof(command), CONFIG_SSD1306_I2C_ADDR);

        if (status != 0)
        {
            return status;
        }
    }

#if (CONFIG_SSD1306_OLED_ROWS == 32)
        for (uint16_t i=0; i < (CONFIG_SSD1306_OLED_COLUMNS * CONFIG_SSD1306_OLED_ROWS / 8); i++) {
	        // send a bunch of data in one xmission
	        Wire.beginTransmission(_i2caddr);
	        Wire.write(0x40);
            for (uint8_t x=0; x<16; x++) {
	            Wire.write((uint8_t)0x00);
	            i++;
	        }
	        i--;
	        Wire.endTransmission();
        }
#endif

    return status;
}

// Clear the contents of the backbuffer
void ssd1306_clear_backbuffer()
{
    memset(s_backBuffer, 0, sizeof(s_backBuffer));
}

void ssd1306_draw_pixel(uint16_t x, uint16_t y, PIXEL_COLOR color)
{
    if (!IS_VALID_PIXEL_X_Y(x, y) || !IS_VALID_PIXEL_COLOR(color))
    {
        return;
    }

    switch (color)
    {
        case FOREGROUND:
            s_backBuffer[x + ((y / 8) * CONFIG_SSD1306_OLED_COLUMNS)] |= (1 << (y & 0xF));
            break;

        case BACKGROUND:
            s_backBuffer[x + ((y / 8) * CONFIG_SSD1306_OLED_COLUMNS)] &= ~(1 << (y & 0xF));
            break;
        default:
            break;
    }
}

struct ssd1306_data ssd1306_driver;

DEVICE_INIT(ssd1306, CONFIG_SSD1306_NAME, ssd1306_init, &ssd1306_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);