#pragma once

#define TIMER_IRQ_PRIORITY 4
#define TIMER_FREQUENCY NRF_TIMER_FREQ_62500Hz
#define DRV8825_PULSE_WIDTH_US 20

#define CONFIG_GPIO_DEV "GPIO_0"

// DRV8825 Instance 0
#define CONFIG_DRV8825_0_NAME "drv8825_0"
#define CONFIG_DRV8825_0_STEP_PIN 14
#define CONFIG_DRV8825_0_DIR_PIN 15
#define CONFIG_DRV8825_0_M0_PIN 31
#define CONFIG_DRV8825_0_M1_PIN 13
#define CONFIG_DRV8825_0_M2_PIN 16

// DRV8825 Instance 1
#define CONFIG_DRV8825_1_NAME "drv8825_1"
#define CONFIG_DRV8825_1_STEP_PIN 4
#define CONFIG_DRV8825_1_DIR_PIN 3
#define CONFIG_DRV8825_1_M0_PIN 30
#define CONFIG_DRV8825_1_M1_PIN 29
#define CONFIG_DRV8825_1_M2_PIN 28