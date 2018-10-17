// System includes
#include <errno.h>
#include <zephyr.h>
#include <board.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <gpio.h>
#include <stdio.h>

// Application includes
#include "messages.h"
#include "ui.h"
#include "display_ssd1306.h"
#include "config_display_ssd1306.h"

// Definitions
#define LCD_DEV "lcd1602"
#define OLED_DEV "ssd1306"
#define GPIO_DEV "GPIO_0"

// Private function declarations
void refresh_display();
void refresh_display_region(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void draw_bitmap(uint8_t x_pos, uint8_t y_pos, uint8_t* buffer, uint8_t width, uint8_t height);

// A simple bitmap of a smiley face
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

// Device pointers
static struct device* oled_dev;
static struct device* gpio_dev;

// GPIO callbacks
static struct gpio_callback sw_callback;

void button_callback(struct device *port, struct gpio_callback *cb, u32_t pins)
{
    // Do not use if -> else if -> else if... here because we can conceivably be called 
    // with multiple buttons pressed!
    // Take care though! We might get called because a new button was pressed, 
    // while an existing button was held down

    if ((pins & BIT(SW0_GPIO_PIN)) == BIT(SW0_GPIO_PIN))
    {
        printf("Sending BT LE Scan request\r\n");

        BLUETOOTH_TASK_MESSAGE message;
        message.command = BLUETOOTH_START_SCAN;

        while (k_msgq_put(&bluetooth_task_queue, &message, K_NO_WAIT) != 0) 
        {
            // message queue is full: purge old data & try again
            k_msgq_purge(&bluetooth_task_queue);
        }
    }
    
    if ((pins & BIT(SW1_GPIO_PIN)) == BIT(SW1_GPIO_PIN))
    {
        printf("Button 1 pressed\r\n");
    }

    if ((pins & BIT(SW2_GPIO_PIN)) == BIT(SW2_GPIO_PIN))
    {
        printf("Button 2 pressed\r\n");
    }

    if ((pins & BIT(SW3_GPIO_PIN)) == BIT(SW3_GPIO_PIN))
    {
        printf("Button 2 pressed\r\n");
    }
}

void ui_task(void *arg1, void *arg2, void *arg3)
{
    // If this device is not found, it is non-fatal.
    // Instead, all draw-calls to the OLED will simply fail silently.
    oled_dev = device_get_binding(OLED_DEV);
    if (!oled_dev)
    {
        // TODO: implement Zephyr Logging system
		printf("WARNING: OLED device driver not found.\r\n");
    }

    gpio_dev = device_get_binding(GPIO_DEV);
    if (!gpio_dev) 
    {
        printf("ERROR: GPIO device driver not found.\r\n");
        return;
    }

    // Configure buttons for the UI
    // TODO: check for configuration errors
    gpio_pin_configure(gpio_dev, SW0_GPIO_PIN, SW0_GPIO_PIN_PUD | GPIO_DIR_IN | GPIO_INT | GPIO_INT_ACTIVE_LOW | GPIO_INT_EDGE);
    gpio_pin_configure(gpio_dev, SW1_GPIO_PIN, SW1_GPIO_PIN_PUD | GPIO_DIR_IN | GPIO_INT | GPIO_INT_ACTIVE_LOW | GPIO_INT_EDGE);
    gpio_pin_configure(gpio_dev, SW2_GPIO_PIN, SW2_GPIO_PIN_PUD | GPIO_DIR_IN | GPIO_INT | GPIO_INT_ACTIVE_LOW | GPIO_INT_EDGE);
    gpio_pin_configure(gpio_dev, SW3_GPIO_PIN, SW3_GPIO_PIN_PUD | GPIO_DIR_IN | GPIO_INT | GPIO_INT_ACTIVE_LOW | GPIO_INT_EDGE);

    gpio_init_callback(&sw_callback, button_callback, BIT(SW0_GPIO_PIN) | BIT(SW1_GPIO_PIN) | BIT(SW2_GPIO_PIN) | BIT(SW3_GPIO_PIN));
    gpio_add_callback(gpio_dev, &sw_callback);

    gpio_pin_enable_callback(gpio_dev, SW0_GPIO_PIN);
    gpio_pin_enable_callback(gpio_dev, SW1_GPIO_PIN);
    gpio_pin_enable_callback(gpio_dev, SW2_GPIO_PIN);
    gpio_pin_enable_callback(gpio_dev, SW3_GPIO_PIN);

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
        refresh_display_region(x, y, 1, 1);
        if (x + dirx < 0 || x + dirx >= CONFIG_SSD1306_OLED_COLUMNS - 1)
        {
            dirx *= -1;
        } else x += dirx;
        if (y + diry < 0 || y + diry >= CONFIG_SSD1306_OLED_ROWS - 1)
        {
            diry *= -1;
        } else y += diry;

        k_sleep(100);
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
void refresh_display()
{
    if (!oled_dev)
    {
        return;
    }

    // Display updates are time critical: they must complete before the display refreshes.
    // Lock out other threads while we update the display
    // TODO: this could impact the balancing code
    k_sched_lock();
    {
        ssd1306_refresh(oled_dev);
    }
    k_sched_unlock();
}

void refresh_display_region(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if (!oled_dev)
    {
        return;
    }

    // Display updates are time critical: they must complete before the display refreshes.
    // Lock out other threads while we update the display
    // TODO: this could impact the balancing code
    k_sched_lock();
    {
        ssd1306_refresh_region(oled_dev, x, y, width, height);
    }
    k_sched_unlock();
}