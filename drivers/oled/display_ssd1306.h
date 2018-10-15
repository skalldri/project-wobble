#ifndef DISPLAY_SSD1306_H
#define DISPLAY_SSD1306_H

#include <zephyr.h>

#include <device.h>
#include <gpio.h>
#include <misc/util.h>
#include <zephyr/types.h>

#define SYS_LOG_DOMAIN "SSD1306"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SSD1306_LEVEL
#include <logging/sys_log.h>

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

// Set the CO bit to indicate that the following bytes will all have
// control bytes before them
#define SSD1306_CONTROL_CO_BIT (1 << 8)
// Set the DC bit to indicate the next byte is a data byte to store in RAM
#define SSD1306_CONTROL_DC_BIT (1 << 7)

typedef enum {
    DISPLAY_ROWS_32,
    DISPLAY_ROWS_64,

    // !!! Insert new display resolutions above this line !!!
    DISPLAY_ROWS_COUNT
} display_rows;

// Fundamentally, the SSD1306 has two colors. A "foreground" color, and a "background" color.
// Normally, the foreground is white and the background is black. 
// The SSD1306 can be commanded to quickly swap the mapping so that foreground is black, and background is white.
// In memory, foreground is always represented as a 1 and background is always represented as a 0.
//
// Users of this enum should _NEVER_ assume that they can use the enum value as the bit value.
// Users should _ALWAYS_ explicitly convert PIXEL_COLOR::BACKGROUND and PIXEL_COLOR::FOREGROUND to 0 and 1.
typedef enum {
    BACKGROUND,
    FOREGROUND,

    // !!! Insert new colors above this line !!!
    PIXEL_COLOR_COUNT
} PIXEL_COLOR;

struct ssd1306_data {
	struct device *i2c;
    display_rows rows;
};

// Helper function which will quickly validate if a pixel co-ordinate 
// is valid inside the backbuffer / on-screen
#define IS_VALID_PIXEL_X_Y(x, y) (((x) <= CONFIG_SSD1306_OLED_COLUMNS) && ((y) <= CONFIG_SSD1306_OLED_ROWS))

#define IS_VALID_PIXEL_COLOR(color) ((color) < PIXEL_COLOR_COUNT)

int ssd1306_refresh(struct device* dev);
void ssd1306_clear_backbuffer();
void ssd1306_draw_pixel(uint16_t x, uint16_t y, PIXEL_COLOR color);

#endif // DISPLAY_SSD1306_H