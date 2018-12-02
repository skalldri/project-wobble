#include "drv8825.h"
#include "config_drv8825.h"

#include <init.h>
#include <irq.h>
#include <nrfx.h>
#include <nrf_timer.h>
#include <nrf_ppi.h>
#include <nrf_gpiote.h>
#include <gpio.h>
#include <stdio.h>

// This enum is used to track the current state of a timer pulse generator.
// While extremely simple, it's useful for keeping track of the current state of the 
// pulse generator.
typedef enum {
    PULSE_STATE_PULSE_ON,   // The timer is currently generating the pulse
    PULSE_STATE_PULSE_OFF,  // The timer is not currently generating the pulse

    // !!! INSERT NEW STATES ABOVE THIS LINE !!!
    TIMER_PULSE_STATE_COUNT  
} TIMER_PULSE_STATE;

struct drv8825_data {
    struct device*      gpio_dev;
    NRF_TIMER_Type*     timer;
    float               desired_pulse_per_second;
    uint32_t            timer_ticks_pulse_on;
    uint32_t            timer_ticks_pulse_off;
    TIMER_PULSE_STATE   state;
    uint32_t            step_pin;
    uint32_t            dir_pin;
    uint32_t            m0_pin;
    uint32_t            m1_pin;
    uint32_t            m2_pin;

    // PPI channels for STEP pin control
    nrf_ppi_channel_t   ppi_channel_a;
    nrf_ppi_channel_t   ppi_channel_b;

    // GPIOTE channel for STEP pin control
    uint32_t            gpiote_channel;
    nrf_gpiote_tasks_t  gpiote_set_task;  
    nrf_gpiote_tasks_t  gpiote_clr_task;
};

struct drv8825_data drv8825_0_driver = {
    .timer = NRF_TIMER0,
    .step_pin = CONFIG_DRV8825_0_STEP_PIN,
    .dir_pin = CONFIG_DRV8825_0_DIR_PIN,
    .m0_pin = CONFIG_DRV8825_0_M0_PIN,
    .m1_pin = CONFIG_DRV8825_0_M1_PIN,
    .m2_pin = CONFIG_DRV8825_0_M2_PIN,
    .ppi_channel_a = NRF_PPI_CHANNEL0,
    .ppi_channel_b = NRF_PPI_CHANNEL1,
    .gpiote_channel = 0,
    .gpiote_set_task = NRF_GPIOTE_TASKS_SET_0,
    .gpiote_clr_task = NRF_GPIOTE_TASKS_CLR_0,
};

struct drv8825_data drv8825_1_driver = {
    .timer = NRF_TIMER1,
    .step_pin = CONFIG_DRV8825_1_STEP_PIN,
    .dir_pin = CONFIG_DRV8825_1_DIR_PIN,
    .m0_pin = CONFIG_DRV8825_1_M0_PIN,
    .m1_pin = CONFIG_DRV8825_1_M1_PIN,
    .m2_pin = CONFIG_DRV8825_1_M2_PIN,
    .ppi_channel_a = NRF_PPI_CHANNEL2,
    .ppi_channel_b = NRF_PPI_CHANNEL3,
    .gpiote_channel = 1,
    .gpiote_set_task = NRF_GPIOTE_TASKS_SET_1,
    .gpiote_clr_task = NRF_GPIOTE_TASKS_CLR_1,
};

int drv8825_program_timer(struct device *dev);

// This IRQ is called at 65khz (see nrf_timer.h for exact frequency)
// The motor driver GPIO should be driven based on this timer
void drv8825_irq_handler(struct drv8825_data *drv_data)
{
    // Stop the timer
    nrf_timer_task_trigger(drv_data->timer, NRF_TIMER_TASK_STOP);
    nrf_timer_task_trigger(drv_data->timer, NRF_TIMER_TASK_CLEAR);

    switch(drv_data->state)
    {
        case PULSE_STATE_PULSE_ON:
            // The GPIO is currently ON
            // Turn OFF the GPIO
            gpio_pin_write(drv_data->gpio_dev, drv_data->step_pin, 0);

            // Reconfigure the timer to re-trigger this IRQ
            // after the OFF duration
            nrf_timer_cc_write(drv_data->timer, NRF_TIMER_CC_CHANNEL0, drv_data->timer_ticks_pulse_off);

            // Switch to the OFF state
            drv_data->state = PULSE_STATE_PULSE_OFF;
            break;

        case PULSE_STATE_PULSE_OFF:
            // The GPIO is currently OFF
            // Turn ON the GPIO
            gpio_pin_write(drv_data->gpio_dev, drv_data->step_pin, 1);

            // Reconfigure the timer to re-trigger this IRQ
            // after the OFF duration
            nrf_timer_cc_write(drv_data->timer, NRF_TIMER_CC_CHANNEL0, drv_data->timer_ticks_pulse_on);

            // Switch to the OFF state
            drv_data->state = PULSE_STATE_PULSE_ON;
            break;

        default:
            __ASSERT(false, "Unknown pulse generator state");
    }

    // Restart the timer now that it has been reprogrammed
    nrf_timer_task_trigger(drv_data->timer, NRF_TIMER_TASK_START);
}

ISR_DIRECT_DECLARE(timer0_irq)
{
    drv8825_irq_handler(&drv8825_0_driver);

    ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency */
    return 1; /* We should check if scheduling decision should be made */
}

ISR_DIRECT_DECLARE(timer1_irq)
{
    drv8825_irq_handler(&drv8825_1_driver);

    ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency */
    return 1; /* We should check if scheduling decision should be made */
}

int drv8825_init(struct device *dev)
{
    struct drv8825_data *drv_data = dev->driver_data;

    // Setup static structures needed for all driver instances
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER0), TIMER_IRQ_PRIORITY, timer0_irq, 0);
    IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_TIMER1), TIMER_IRQ_PRIORITY, timer1_irq, 0);

    irq_enable(NRFX_IRQ_NUMBER_GET(drv_data->timer));

    drv_data->gpio_dev = device_get_binding(CONFIG_GPIO_DEV);
    if (!drv_data->gpio_dev) 
    {
        printf("ERROR: GPIO device driver not found.\r\n");
        return -ENODEV;
    }
    
    // Configure timers to run in TIMER mode
    nrf_timer_mode_set(drv_data->timer, NRF_TIMER_MODE_TIMER);
    
    // Configure the timers to run at the maximum available bit-width
    nrf_timer_bit_width_set(drv_data->timer, NRF_TIMER_BIT_WIDTH_32);

    // Configure timer to run at 62500 Hz
    nrf_timer_frequency_set(drv_data->timer, TIMER_FREQUENCY);

    // Reset the timer when CC1 is triggered
    nrf_timer_shorts_enable(drv_data->timer, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK);

    // When CC0 is triggered, turn off the STEP GPIO
    nrf_ppi_channel_endpoint_setup(
        drv_data->ppi_channel_a,
        nrf_timer_event_address_get(drv_data->timer, NRF_TIMER_EVENT_COMPARE0),
        nrf_gpiote_task_addr_get(drv_data->gpiote_clr_task));

    // When CC1 is triggered, turn on the STEP GPIO
    nrf_ppi_channel_endpoint_setup(
        drv_data->ppi_channel_a,
        nrf_timer_event_address_get(drv_data->timer, NRF_TIMER_EVENT_COMPARE1),
        nrf_gpiote_task_addr_get(drv_data->gpiote_set_task));

    // Configure the GPIOTE channel to change the correct GPIO pin
    // The last argument is the action GPIOTE should take when it receives
    // the "OUT" task. Since we don't use the OUT task, this value doesn't matter.
    nrf_gpiote_event_configure(drv_data->gpiote_channel, drv_data->step_pin, NRF_GPIOTE_POLARITY_TOGGLE);

    // Claim the GPIO pin specified in nrf_gpiote_event_configure() for use by the GPIOTE module.
    // The GPIO pin can no longer be modified by the regular GPIO interface.
    nrf_gpiote_task_enable(drv_data->gpiote_channel);

    // Configure timer interrupts on CC[0] (we only need one CC register)
    //nrf_timer_int_enable(drv_data->timer, NRF_TIMER_INT_COMPARE0_MASK);

    // Initialize the timer structures to the "STOPPED" state

    // TODO: init GPIO pins to enable microstepping mode
    // TODO: what is the correct microstepping mode? 1/2 step? 1/4 step?
    //gpio_pin_configure(drv_data->gpio_dev, drv_data->step_pin, GPIO_DIR_OUT);
    gpio_pin_configure(drv_data->gpio_dev, drv_data->dir_pin, GPIO_DIR_OUT);
    gpio_pin_configure(drv_data->gpio_dev, drv_data->m0_pin, GPIO_DIR_OUT);
    gpio_pin_configure(drv_data->gpio_dev, drv_data->m1_pin, GPIO_DIR_OUT);
    gpio_pin_configure(drv_data->gpio_dev, drv_data->m2_pin, GPIO_DIR_OUT);

    /* device_get_binding checks if driver_api is not zero before checking
	 * device name.
	 * So just set driver_api to 1 else the function call will fail
     * TODO: add a read Character LCD interface
	 */
	dev->driver_api = (void *)1;

    return 0;
}

int drv8825_set_pulse_per_second(struct device *dev, float pulse_per_second)
{
    struct drv8825_data *drv_data = dev->driver_data;

    drv_data->timer_ticks_pulse_on = nrf_timer_us_to_ticks(DRV8825_PULSE_WIDTH_US, TIMER_FREQUENCY);
    
    // Pulse per second = Frequency Hz
    // Period S = 1 / Frequency Hz
    // Period uS = Period S * 1000000

    drv_data->timer_ticks_pulse_off = nrf_timer_us_to_ticks(((1.0f / (float) pulse_per_second) * 1000000) - DRV8825_PULSE_WIDTH_US, TIMER_FREQUENCY);

    // TODO: update timer CC registers

    return 0;
}

int drv8825_stop(struct device *dev)
{
    struct drv8825_data *drv_data = dev->driver_data;

    // Stop the timer
    nrf_timer_task_trigger(drv_data->timer, NRF_TIMER_TASK_STOP);
    nrf_timer_task_trigger(drv_data->timer, NRF_TIMER_TASK_CLEAR);
    
    // Clear the GPIO
    nrf_gpiote_task_set(drv_data->gpiote_clr_task);
    
    return 0;
}

int drv8825_start(struct device *dev)
{
    struct drv8825_data *drv_data = dev->driver_data;

    // The driver always starts in the "delay" state
    drv_data->state = PULSE_STATE_PULSE_OFF;

    // Ensure the timer is Stopped and reset
    drv8825_stop(dev);

    // Program the capture-compare limits
    drv8825_program_timer(dev);

    // Initially set the GPIO
    nrf_gpiote_task_set(drv_data->gpiote_set_task);

    // Start the timer
    nrf_timer_task_trigger(drv_data->timer, NRF_TIMER_TASK_START);

    return 0;
}

int drv8825_program_timer(struct device *dev)
{
    struct drv8825_data *drv_data = dev->driver_data;

    /*
    The pulse we want to make looks like this:
     _____
    |     |_________
    A     B         C

    This can be generated from a couple few discrete events:

    A: t=0, GPIO on
    B: t=20uS, GPIO off
    C: t=(period - 20uS), reset timer

    Interestingly, because this is continuous, events A and C are the same thing.

    */

    // CC0 is event B. GPIO off.
    nrf_timer_cc_write(drv_data->timer, NRF_TIMER_CC_CHANNEL0, drv_data->timer_ticks_pulse_on);

    // CC1 is event C. GPIO on, and reset the timer
    nrf_timer_cc_write(drv_data->timer, NRF_TIMER_CC_CHANNEL1, drv_data->timer_ticks_pulse_off);

    return 0;
}

int drv8825_set_direction(struct device *dev, MOTOR_DIRECTION dir)
{
    struct drv8825_data *drv_data = dev->driver_data;

    if (dir == DIRECTION_CLOCKWISE)
    {
        gpio_pin_write(drv_data->gpio_dev, drv_data->dir_pin, 0);
    }
    else
    {
        gpio_pin_write(drv_data->gpio_dev, drv_data->dir_pin, 1);
    }

    return 0;
}

DEVICE_INIT(drv8825_0, CONFIG_DRV8825_0_NAME, drv8825_init, &drv8825_0_driver,
		    NULL, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);

DEVICE_INIT(drv8825_1, CONFIG_DRV8825_1_NAME, drv8825_init, &drv8825_1_driver,
		    NULL, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
