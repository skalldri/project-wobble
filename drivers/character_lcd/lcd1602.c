
#include <i2c.h>
#include <init.h>
#include <sensor.h>

#include "lcd1602.h"

// Write a byte to the device to set the pins on the GPIO expander
// Inject the backlight pin into the write if the backlight is supposed to be enabled
int lcd1602_gpio_expander_write(struct device *dev, uint8_t data)
{
    struct lcd1602_data *drv_data = dev->driver_data;

    if (drv_data->backlight)
    {
        data |= LCD1602_BACKLIGHT_MASK;
    }

    return i2c_write(drv_data->i2c, &data, sizeof(uint8_t), CONFIG_LCD1602_I2C_ADDR);
}

// Write a byte of data to the LCD by writing the data and pulsing the EN pin
int lcd1602_pulse_enable(struct device *dev, uint8_t data)
{
    int status = 0;
    
    status = lcd1602_gpio_expander_write(dev, data | LCD1602_ENABLE_MASK);	// En high

    if (status != 0)
    {
        return status;
    }

	k_busy_wait(10);		                                        // enable pulse must be >450ns
	
	status = lcd1602_gpio_expander_write(dev, data & ~LCD1602_ENABLE_MASK);	// En low

    if (status != 0)
    {
        return status;
    }

	k_busy_wait(100);		                                    // commands need > 37us to settle

    return 0;
}

// Write 4 bits of data to the display, strobing the EN pin correctly
int lcd1602_write_four_bits(struct device *dev, uint8_t data)
{
    int status = 0;
    
    status = lcd1602_gpio_expander_write(dev, data);

    if (status != 0)
    {
        return status;
    }

    return lcd1602_pulse_enable(dev, data);
}

// Write 1 byte to the display, 4 bits at a time, with correct strobing
int lcd1602_send(struct device *dev, uint8_t data, uint8_t mode)
{
    int status = 0;
    uint8_t high = data & 0xF0;
	uint8_t low = (data << 4) & 0xF0;
	
    status = lcd1602_write_four_bits(dev, high | mode);

    if (status != 0)
    {
        return status;
    }

	return lcd1602_write_four_bits(dev, low | mode); 
}

int lcd1602_command(struct device *dev, uint8_t command)
{
    return lcd1602_send(dev, command, 0);
}

int lcd1602_clear(struct device *dev)
{
    int status = lcd1602_command(dev, LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	k_sleep(2);  // this command takes a long time!

    return status;
}

int lcd1602_home(struct device *dev)
{
    int status = lcd1602_command(dev, LCD_RETURNHOME);// clear display, set cursor position to zero
	k_sleep(2);  // this command takes a long time!
    
    return status;
}

int lcd1602_set_cursor(struct device *dev, uint8_t row, uint8_t column)
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };

	if ( row > (CONFIG_LCD1602_LCD_ROWS - 1) ) 
    {
		row = CONFIG_LCD1602_LCD_ROWS - 1;    // we count rows starting w/0
	}

	return lcd1602_command(dev, LCD_SETDDRAMADDR | (column + row_offsets[row]));
}

// Turn the display on/off (quickly)
int lcd1602_display_enable(struct device *dev, bool enabled) 
{
    struct lcd1602_data *drv_data = dev->driver_data;

    if (enabled)
    {
        drv_data->displayControl |= LCD_DISPLAYON;
    }
    else
    {
        drv_data->displayControl &= ~LCD_DISPLAYON;
    }

	return lcd1602_command(dev, LCD_DISPLAYCONTROL | drv_data->displayControl);
}

int lcd1602_write(struct device *dev, char ch)
{
    return lcd1602_send(dev, ch, LCD1602_REG_SELECT_MASK);
}

int lcd1602_set_backlight(struct device *dev, bool enabled)
{
    struct lcd1602_data *drv_data = dev->driver_data;

    drv_data->backlight = enabled;
    return lcd1602_gpio_expander_write(dev, 0);
}

int lcd1602_init(struct device *dev)
{
    struct lcd1602_data *drv_data = dev->driver_data;

    // Setup display data structures
    drv_data->backlight = false;
    drv_data->displayFunctions = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

    if (CONFIG_LCD1602_LCD_ROWS > 1)
    {
        drv_data->displayFunctions |= LCD_2LINE;
    }

    drv_data->i2c = device_get_binding(CONFIG_LCD1602_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device",
			    CONFIG_LCD1602_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

    // Perform an initial write to reset the GPIO expander
    if (lcd1602_gpio_expander_write(dev, 0) != 0)
    {
        SYS_LOG_ERR("Failed to perform initial write to IO expander");
        return -EINVAL;
    }

    lcd1602_write_four_bits(dev, 0x03 << 4);
    k_sleep(5); // wait min 4.1ms
   
    // second try
    lcd1602_write_four_bits(dev, 0x03 << 4);
    k_sleep(5); // wait min 4.1ms
   
    // third go!
    lcd1602_write_four_bits(dev, 0x03 << 4); 
    k_busy_wait(150);
   
    // finally, set to 4-bit interface
    lcd1602_write_four_bits(dev, 0x02 << 4); 

    // set # lines, font size, etc.
	lcd1602_command(dev, LCD_FUNCTIONSET | drv_data->displayFunctions);  
	
	// turn the display on with no cursor or blinking default
	drv_data->displayControl = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	lcd1602_display_enable(dev, true);
	
	// clear it off
	lcd1602_clear(dev);
	
	// Initialize to default text direction (for roman languages)
	drv_data->displayMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	lcd1602_command(dev, LCD_ENTRYMODESET | drv_data->displayMode);
	
	lcd1602_home(dev);

    /* device_get_binding checks if driver_api is not zero before checking
	 * device name.
	 * So just set driver_api to 1 else the function call will fail
     * TODO: add a read Character LCD interface
	 */
	dev->driver_api = (void *)1;

    return 0;
}

struct lcd1602_data lcd1602_driver;

DEVICE_INIT(lcd1602, CONFIG_LCD1602_NAME, lcd1602_init, &lcd1602_driver,
		    NULL, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);