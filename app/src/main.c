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

#define IMU_TASK_STACK_SIZE (512)
K_THREAD_STACK_DEFINE(imu_task_stack, IMU_TASK_STACK_SIZE);
static struct k_thread imu_data;

#define TEMPERATURE_TASK_STACK_SIZE (512)
K_THREAD_STACK_DEFINE(temperature_task_stack, TEMPERATURE_TASK_STACK_SIZE);
static struct k_thread temperature_data;

#define UI_TASK_STACK_SIZE (512)
K_THREAD_STACK_DEFINE(ui_task_stack, UI_TASK_STACK_SIZE);
static struct k_thread ui_data;

void main(void)
{
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
}