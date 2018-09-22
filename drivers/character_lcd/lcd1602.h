#ifndef LCD1602_H
#define LCD1602_H

#include <zephyr.h>

#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <zephyr/types.h>

#define SYS_LOG_DOMAIN "LCD1602"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LCD1602_LEVEL
#include <logging/sys_log.h>

// Bitmasks for various GPIO pins on the GPIO expander
#define LCD1602_BACKLIGHT_MASK  0x08    // Backlight pin
#define LCD1602_ENABLE_MASK     0x04    // Enable bit
#define LCD1602_RW_MASK         0x02    // Read/Write bit
#define LCD1602_REG_SELECT_MASK 0x01    // Register select bit

// commands
#define LCD_CMD_CLEAR_DISPLAY   0x01
#define LCD_CMD_RETURN_HOME     0x02
#define LCD_CMD_ENTRY_MODE_SET  0x04
#define LCD_CMD_DISPLAY_CONTROL 0x08
#define LCD_CMD_CURSOR_SHIFT    0x10
#define LCD_CMD_FUNCTION_SET    0x20
#define LCD_CMD_SET_CGRAM_ADDR  0x40
#define LCD_CMD_SET_DDRAM_ADDR  0x80

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

struct lcd1602_data {
	struct device *i2c;
    bool backlight;
    uint8_t displayFunctions;
    uint8_t displayControl;
    uint8_t displayMode;
};

int lcd1602_write(struct device *dev, char ch);
int lcd1602_set_backlight(struct device *dev, bool enabled);

#endif // LCD1602_H