#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include "imu.h"
#include "temperature.h"
#include "ui.h"
#include "bt.h"

#include "nrf52.h"

#define IMU_TASK_STACK_SIZE (2048)
K_THREAD_STACK_DEFINE(imu_task_stack, IMU_TASK_STACK_SIZE);
static struct k_thread imu_data;

#define TEMPERATURE_TASK_STACK_SIZE (2048)
K_THREAD_STACK_DEFINE(temperature_task_stack, TEMPERATURE_TASK_STACK_SIZE);
static struct k_thread temperature_data;

#define UI_TASK_STACK_SIZE (2048)
K_THREAD_STACK_DEFINE(ui_task_stack, UI_TASK_STACK_SIZE);
static struct k_thread ui_data;

#define BLUETOOTH_TASK_STACK_SIZE (2048)
K_THREAD_STACK_DEFINE(bluetooth_task_stack, BLUETOOTH_TASK_STACK_SIZE);
static struct k_thread bluetooth_data;

void main(void)
{
	// Disable pin reset
	/*NRF_UICR->PSELRESET[0] = 0x0;
	NRF_UICR->PSELRESET[1] = 0x1;*/

	k_thread_create(&temperature_data, 						
					temperature_task_stack,					// Data buffer for thread stack
					TEMPERATURE_TASK_STACK_SIZE,			// Thread stack size 
					(k_thread_entry_t) temperature_task, 	// Thread function
					NULL, 									// Parameter 1
					NULL, 									// Parameter 2
					NULL, 									// Parameter 3
					K_PRIO_COOP(7),							// Thread priority 
					0, 										// Thread options
					K_NO_WAIT);								// Scheduling delay

	k_thread_create(&imu_data, 						
					imu_task_stack,							// Data buffer for thread stack
					IMU_TASK_STACK_SIZE,					// Thread stack size 
					(k_thread_entry_t) imu_task, 			// Thread function
					NULL, 									// Parameter 1
					NULL, 									// Parameter 2
					NULL, 									// Parameter 3
					K_PRIO_COOP(7),							// Thread priority 
					0, 										// Thread options
					K_NO_WAIT);								// Scheduling delay

	k_thread_create(&ui_data, 						
					ui_task_stack,							// Data buffer for thread stack
					UI_TASK_STACK_SIZE,						// Thread stack size 
					(k_thread_entry_t) ui_task, 			// Thread function
					NULL, 									// Parameter 1
					NULL, 									// Parameter 2
					NULL, 									// Parameter 3
					K_PRIO_COOP(7),							// Thread priority 
					0, 										// Thread options
					K_NO_WAIT);								// Scheduling delay
	
	k_thread_create(&bluetooth_data, 						
					bluetooth_task_stack,					// Data buffer for thread stack
					BLUETOOTH_TASK_STACK_SIZE,			    // Thread stack size 
					(k_thread_entry_t) bluetooth_task,   	// Thread function
					NULL, 									// Parameter 1
					NULL, 									// Parameter 2
					NULL, 									// Parameter 3
					K_PRIO_COOP(7),							// Thread priority 
					0, 										// Thread options
					K_NO_WAIT);								// Scheduling delay
}