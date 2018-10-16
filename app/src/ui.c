#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include <display_ssd1306.h>
#include <config_display_ssd1306.h>

#include "ui.h"

// Definitions
#define LCD_DEV "lcd1602"
#define OLED_DEV "ssd1306"

// Private function declarations
void refresh_display(struct device* oled_dev);
void draw_bitmap(uint8_t x_pos, uint8_t y_pos, uint8_t* buffer, uint8_t width, uint8_t height);

/*
A simple bitmap of a smiley face:

00000000b, 00000000b,
00000011b, 11000000b,
00001100b, 00110000b,
00110000b, 00001100b,
01000110b, 01100010b,
01000110b, 01100010b,
01000000b, 00000010b,
01000000b, 00000010b,
01001000b, 00010010b,
01000111b, 11100010b,
00110000b, 00001100b,
00001100b, 00110000b,
00000011b, 11000000b,
00000000b, 00000000b,
00000000b, 00000000b,
00000000b, 00000000b,

*/

static uint8_t s_smileyBitmap[] = {
    0B00000000, 0B00000000,
    0B00000011, 0B11000000,
    0B00001100, 0B00110000,
    0B00110000, 0B00001100,
    0B01000110, 0B01100010,
    0B01000110, 0B01100010,
    0B01000000, 0B00000010,
    0B01000000, 0B00000010,
    0B01001000, 0B00010010,
    0B01000111, 0B11100010,
    0B00110000, 0B00001100,
    0B00001100, 0B00110000,
    0B00000011, 0B11000000,
    0B00000000, 0B00000000,
    0B00000000, 0B00000000,
    0B00000000, 0B00000000
};


void ui_task(void *arg1, void *arg2, void *arg3)
{
    struct device* oled_dev;

	printf("\r\n");

    oled_dev = device_get_binding(OLED_DEV);
    if (!oled_dev) {
		printf("OLED: Device driver not found.\r\n");
        return;
    }

    // Print the back-buffer logo
    refresh_display(oled_dev);
    k_sleep(1000);
    //ssd1306_clear_backbuffer();

    int x = 0, y = 0;
    int dirx = 1, diry = 1;
    while(true)
    {
        ssd1306_draw_pixel(x, y, FLIP_COLOR);
        //ssd1306_clear_backbuffer();
        //draw_bitmap(x, y, s_smileyBitmap, 16, 16);
        //refresh_display(oled_dev);
        k_sched_lock();
        {
            ssd1306_refresh_region(oled_dev, x, y, 1, 1);
        }
        k_sched_unlock();
        if (x + dirx < 0 || x + dirx >= CONFIG_SSD1306_OLED_COLUMNS - 1)
        {
            dirx *= -1;
        } else x += dirx;
        if (y + diry < 0 || y + diry >= CONFIG_SSD1306_OLED_ROWS - 1)
        {
            diry *= -1;
        } else y += diry;
    }
}

// Draw a bitmap at a given X/Y co-ordinate on the screen
// The X/Y co-ordinate should indicate the top-left corner of the bitmap to draw
// buffer should be size (width * height) / 8, since each bit in each byte represents one pixel.
void draw_bitmap(uint8_t x_pos, uint8_t y_pos, uint8_t* buffer, uint8_t width, uint8_t height)
{
    // For each byte in the buffer...
    for (uint32_t i = 0; i < ((width * height) / 8); i++)
    {
        // For each bit in the current byte
        for (uint8_t j = 0; j < 8; j++)
        {
            // Compute the position of this bit in the output buffer.
            uint16_t out_x = (((i * 8) + j) % width) + x_pos;
            uint16_t out_y = (((i * 8) + j) / width) + y_pos;
            PIXEL_COLOR color = ((buffer[i] >> (7 - j)) & 0x1) == 0x1 ? FOREGROUND : BACKGROUND;

            ssd1306_draw_pixel(out_x, out_y, color);
        }
    }
}

// Helper function which ensures we update the display without being interrupted by another thread
// WARNING: This process takes ~30ms!!!! Do not update the display
void refresh_display(struct device* oled_dev)
{
    // Display updates are time critical: they must complete before the display refreshes.
    // Lock out other threads while we update the display
    // TODO: this could impact the balancing code
    k_sched_lock();
    {
        ssd1306_refresh(oled_dev);
    }
    k_sched_unlock();
}