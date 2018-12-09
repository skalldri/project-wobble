#include <errno.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <stdio.h>
#include <math.h>

#include "imu.h"
#include "drv8825.h"
#include "arm_math.h"

#define I2C_DEV CONFIG_I2C_0_NAME
#define CONFIG_MPU9250_NAME "mpu9250"
#define CONFIG_DRV8825_0_NAME "drv8825_0"
#define CONFIG_DRV8825_1_NAME "drv8825_1"

#define ENABLE_LOG_PID
#define ENABLE_LOG_ANGLE

#ifdef ENABLE_LOG_ANGLE
#define LOG_ANGLE(s, ...) printf("ANGLE: " s, ##__VA_ARGS__)
#else
#define LOG_ANGLE(s, ...)
#endif

#ifdef ENABLE_LOG_PID
#define LOG_PID(s, ...) printf("PID: " s, ##__VA_ARGS__)
#else
#define LOG_PID(s, ...)
#endif

#define USE_GYRO_CAL

#define LOOP_MS 10

#define GYRO_CAL_ITERATIONS 500

#define GYRO_PITCH_AXIS 0
#define GYRO_YAW_AXIS 1

#define MAX_PID_OUTPUT 2000
#define PID_DEADZONE 10
#define MAX_SELF_BALANCE_ADJUST 2.0
#define NATURAL_BALANCE_ANGLE 3.17

#define LEFT_MOTOR_FORWARD		DIRECTION_CLOCKWISE
#define LEFT_MOTOR_BACKWARD		DIRECTION_COUNTER_CLOCKWISE

#define RIGHT_MOTOR_FORWARD		DIRECTION_COUNTER_CLOCKWISE
#define RIGHT_MOTOR_BACKWARD	DIRECTION_CLOCKWISE

#define K_P 350.0
#define K_I 1.5
#define K_D 200.0

typedef struct _PID_CONTEXT {
	double k_p, k_i, k_d;		// PID constants

	bool run;					// Boolean which indicates if the system should be running
	
	double gyro_yaw_cal_value;	// Computed during startup, this is the "noise" in the sensor
	double gyro_pitch_cal_value; // Computed during startup, this is the "noise" in the sensor

	double angle_acc;			// Current angle computed from the accelerometer. Used to bootstrap the balancing loop during startup. Not succecptible to drift.
	double angle_gyro;			// Current angle computed from the gyro. Uses gyro delta samples to dead-reckon the current angle. Succeptible to drift.

	double self_balance_pid_setpoint; 	// Tracks the desired angle of the PID controller at rest. Auto-adjusts to keep the robot stationary
	double pid_setpoint;					// User input PID setpoint, used for causing the robot to drive forwards or backwards

	double pid_i_mem;
	double pid_last_d_error;

	double pid_output;

	uint32_t motor_left_sps;		// Motor 0 desired steps-per-second
	uint32_t motor_right_sps;		// Motor 0 desired steps-per-second
	
	MOTOR_DIRECTION motor_0_direction;	
	MOTOR_DIRECTION motor_1_direciton;

} PID_CONTEXT;

static PID_CONTEXT pid_context;

void imu_task(void *arg1, void *arg2, void *arg3)
{
    struct device* i2c_dev;
	struct device* motor_left_dev;
	struct device* motor_right_dev;
	struct sensor_value sensor_values[3] = {0};
	struct sensor_value gravity_constant;

	printf("\r\n");

	i2c_dev = device_get_binding(CONFIG_MPU9250_NAME); //pointer to a device
	if (!i2c_dev) {
		printf("ERROR: IMU Device driver not found.\r\n");
		//return;
	}

	motor_left_dev = device_get_binding(CONFIG_DRV8825_0_NAME); //pointer to a device
	if (!motor_left_dev) {
		printf("ERROR: Motor Driver 0 Device driver not found.\r\n");
		//return;
	}

	motor_right_dev = device_get_binding(CONFIG_DRV8825_1_NAME); //pointer to a device
	if (!motor_right_dev) {
		printf("ERROR: Motor Driver 1 Device driver not found.\r\n");
		//return;
	}

	/*drv8825_set_pulse_per_second(motor_left_dev, MAX_PID_OUTPUT);
	drv8825_set_direction(motor_left_dev, LEFT_MOTOR_FORWARD);
	drv8825_start(motor_left_dev);

	drv8825_set_pulse_per_second(motor_right_dev, MAX_PID_OUTPUT);
	drv8825_set_direction(motor_right_dev, RIGHT_MOTOR_FORWARD);
	drv8825_start(motor_right_dev);*/

	const double gravity = ((double)SENSOR_G) / 1000000.0;
	pid_context.k_p = K_P;
	//pid_context.k_d = K_D;

	//static uint32_t iterations = 0;

#if defined(USE_GYRO_CAL)
	/*if (i2c_dev)
	{
		printf("Calibrating gyro, please wait\r\n");

		for(uint32_t i = 0; i < GYRO_CAL_ITERATIONS; i++)
		{       
			sensor_sample_fetch(i2c_dev); // gets next sample from driver

			sensor_channel_get(i2c_dev, SENSOR_CHAN_GYRO_XYZ, sensor_values);

			pid_context.gyro_pitch_cal_value += sensor_value_to_double(&sensor_values[GYRO_PITCH_AXIS]);
			pid_context.gyro_yaw_cal_value += sensor_value_to_double(&sensor_values[GYRO_YAW_AXIS]);

			k_sleep(4);
		}

		pid_context.gyro_pitch_cal_value /= GYRO_CAL_ITERATIONS;
  		pid_context.gyro_yaw_cal_value /= GYRO_CAL_ITERATIONS;      

		printf("Calibration complete\r\n");
		printf("gyro_pitch_cal_value: %f\r\n", pid_context.gyro_pitch_cal_value);
		printf("gyro_yaw_cal_value: %f\r\n", pid_context.gyro_yaw_cal_value);
	}*/

	pid_context.gyro_pitch_cal_value = 0.026961;
#endif

	

	while(true) 
	{
		if (i2c_dev)
		{
			sensor_sample_fetch(i2c_dev); // gets next sample from driver

			sensor_channel_get(i2c_dev, SENSOR_CHAN_ACCEL_XYZ, sensor_values);

			/*LOG_ANGLE("accel x: %e\r\n", sensor_value_to_double(&sensor_values[0]));
			LOG_ANGLE("accel y: %e\r\n", sensor_value_to_double(&sensor_values[1]));
			LOG_ANGLE("accel z: %e\r\n", sensor_value_to_double(&sensor_values[2]));*/

			double accel_data = sensor_value_to_double(&sensor_values[2]); // Z is the balancing channel, trying to make this close to 0
			//printf("accel_data y: %e\r\n", accel_data);

			sensor_channel_get(i2c_dev, SENSOR_CHAN_GYRO_XYZ, sensor_values);
			/*LOG_ANGLE("gyro x: %f\r\n", sensor_value_to_double(&sensor_values[0]) * 57.2958f); // X is the balancing channel
			LOG_ANGLE("gyro y: %f\r\n", sensor_value_to_double(&sensor_values[1]) * 57.2958f);
			LOG_ANGLE("gyro z: %f\r\n", sensor_value_to_double(&sensor_values[2]) * 57.2958f);*/

			// Clamp accelerometer data to the gravity constant since asin() doesn't accept values > 1
			if (accel_data > gravity)
			{
				accel_data = gravity;
			}
			else if (accel_data < (-1.0f * gravity))
			{
				accel_data = (-1.0f * gravity);
			}

			// Convert radians to degrees
			pid_context.angle_acc = asin(accel_data / gravity) * 57.2958f;

			// If the platform is almost balanced, and the balancing routine is not started,
			// load the current angle into the gyro angle and start balancing
			if ((pid_context.run != true && (pid_context.angle_acc < 0.5f) && (pid_context.angle_acc > -0.5f)))
			{
				pid_context.angle_gyro = pid_context.angle_acc;
				pid_context.run = true;

				LOG_ANGLE("balancing starting\r\n");
			}

			double gyro_raw = sensor_value_to_double(&sensor_values[GYRO_PITCH_AXIS]);

#if defined(USE_GYRO_CAL)
			gyro_raw -= pid_context.gyro_pitch_cal_value;
#endif

			pid_context.angle_gyro += -1.0 * gyro_raw * 57.2958 * (1.0f / 1000.0f) * LOOP_MS;

#if 0
			float degs = sensor_value_to_double(&sensor_values[GYRO_PITCH_AXIS]);

			if (degs > 0)
			{
				drv8825_set_pulse_per_second(motor_right_dev, (degs * 57.2958f) / 1.8f);
				drv8825_set_direction(motor_right_dev, RIGHT_MOTOR_FORWARD);
				drv8825_start(motor_right_dev);
			}
			else if (degs < 0)
			{
				drv8825_set_pulse_per_second(motor_right_dev, ((-1.0f * degs) * 57.2958f) / 1.8f);
				drv8825_set_direction(motor_right_dev, RIGHT_MOTOR_BACKWARD);
				drv8825_start(motor_right_dev);
			}
			else
			{
				drv8825_stop(motor_right_dev);
			}

			k_sleep(LOOP_MS);
			continue;
#endif

			// Correct for mounting offset by correcting with the accelerometer data
			// TODO: MPU offset correction based on Yaw axis of gyro
			pid_context.angle_gyro = pid_context.angle_gyro * 0.9996f + pid_context.angle_acc * 0.0004f;
			//pid_context.angle_gyro = pid_context.angle_acc * 0.9996f + pid_context.angle_gyro * 0.0004f;

			LOG_ANGLE("angle_acc:     %f\r\n", pid_context.angle_acc);
			LOG_ANGLE("angle_gyro:    %f\r\n", pid_context.angle_gyro);

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//PID controller calculations
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//The balancing robot is angle driven. First the difference between the desired angel (setpoint) and actual angle (process value)
			//is calculated. The self_balance_pid_setpoint variable is automatically changed to make sure that the robot stays balanced all the time.
			//The (pid_setpoint - pid_output * 0.015) part functions as a brake function.
			double pid_error_temp = pid_context.angle_gyro - pid_context.self_balance_pid_setpoint - pid_context.pid_setpoint;

			/*if(pid_context.pid_output > (2 * PID_DEADZONE) || pid_context.pid_output < (-1 * PID_DEADZONE))
			{
				LOG_PID("PID braking\r\n");
				pid_error_temp += pid_context.pid_output * 0.015;
			}*/

			LOG_PID("pid_error_temp: %f\r\n", pid_error_temp);
			pid_context.pid_i_mem += pid_context.k_i * pid_error_temp;  //Calculate the I-controller value and add it to the pid_i_mem variable

			//Limit the I-controller to the maximum controller output
			if(pid_context.pid_i_mem > MAX_PID_OUTPUT)
			{
				pid_context.pid_i_mem = MAX_PID_OUTPUT;	
			}                                       
			else if(pid_context.pid_i_mem < -MAX_PID_OUTPUT)
			{
				pid_context.pid_i_mem = -MAX_PID_OUTPUT;
			}

			//Calculate the PID output value
			pid_context.pid_output = pid_context.k_p * pid_error_temp + pid_context.pid_i_mem + pid_context.k_d * (pid_error_temp - pid_context.pid_last_d_error);

			// Limit the PI-controller to the maximum controller output
			if(pid_context.pid_output > MAX_PID_OUTPUT)
			{
				pid_context.pid_output = MAX_PID_OUTPUT; 
			}                                     
			else if(pid_context.pid_output < -MAX_PID_OUTPUT)
			{
				pid_context.pid_output = -MAX_PID_OUTPUT;
			}

			pid_context.pid_last_d_error = pid_error_temp; //Store the error for the next loop

			if(pid_context.pid_output < PID_DEADZONE && pid_context.pid_output > -PID_DEADZONE)
			{
				LOG_PID("PID deadzone: %f\r\n", pid_context.pid_output);
				pid_context.pid_output = 0; //Create a dead-band to stop the motors when the robot is balanced
			}                  

			//If the robot tips over or the start variable is zero or the battery is empty
			if(pid_context.angle_gyro > 30.0f || pid_context.angle_gyro < -30.0f || pid_context.run == false)
			{
				LOG_ANGLE("balancing stopped\r\n");
				pid_context.pid_output = 0;                		//Set the PID controller output to 0 so the motors stop moving
				pid_context.pid_i_mem = 0;                 		//Reset the I-controller memory
				pid_context.run = false;                   		//Set the start variable to 0
				pid_context.self_balance_pid_setpoint = NATURAL_BALANCE_ANGLE; 	//Reset the self_balance_pid_setpoint variable
			}

			LOG_PID("pid_output: %f\r\n", pid_context.pid_output);

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  			//Control calculations
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

			double pid_output_left = pid_context.pid_output;		//Copy the controller output to the pid_output_left variable for the left motor
			double pid_output_right = pid_context.pid_output;	//Copy the controller output to the pid_output_right variable for the right motor
/*
			if(received_byte & B00000001){                                            //If the first bit of the receive byte is set change the left and right variable to turn the robot to the left
				pid_output_left += turning_speed;                                       //Increase the left motor speed
				pid_output_right -= turning_speed;                                      //Decrease the right motor speed
			}
			if(received_byte & B00000010){                                            //If the second bit of the receive byte is set change the left and right variable to turn the robot to the right
				pid_output_left -= turning_speed;                                       //Decrease the left motor speed
				pid_output_right += turning_speed;                                      //Increase the right motor speed
			}

			if(received_byte & B00000100){                                            //If the third bit of the receive byte is set change the left and right variable to turn the robot to the right
				if(pid_setpoint > -2.5)pid_setpoint -= 0.05;                            //Slowly change the setpoint angle so the robot starts leaning forewards
				if(pid_output > max_target_speed * -1)pid_setpoint -= 0.005;            //Slowly change the setpoint angle so the robot starts leaning forewards
			}
			if(received_byte & B00001000){                                            //If the forth bit of the receive byte is set change the left and right variable to turn the robot to the right
				if(pid_setpoint < 2.5)pid_setpoint += 0.05;                             //Slowly change the setpoint angle so the robot starts leaning backwards
				if(pid_output < max_target_speed)pid_setpoint += 0.005;                 //Slowly change the setpoint angle so the robot starts leaning backwards
			}   

			if(!(received_byte & B00001100)){                                         //Slowly reduce the setpoint to zero if no foreward or backward command is given
				if(pid_setpoint > 0.5)pid_setpoint -=0.05;                              //If the PID setpoint is larger then 0.5 reduce the setpoint with 0.05 every loop
				else if(pid_setpoint < -0.5)pid_setpoint +=0.05;                        //If the PID setpoint is smaller then -0.5 increase the setpoint with 0.05 every loop
				else pid_setpoint = 0;                                                  //If the PID setpoint is smaller then 0.5 or larger then -0.5 set the setpoint to 0
			}
*/
			// The self balancing point is adjusted when there is not forward or backwards movement from the transmitter. 
			// This way the robot will always find it's balancing point
			// If the setpoint is zero degrees...
			if(pid_context.pid_setpoint == 0.0)
			{                                                    
				if (pid_context.pid_output < 0.0)
				{
					pid_context.self_balance_pid_setpoint += 0.015;                  //Increase the self_balance_pid_setpoint if the robot is still moving forewards
				}
				else if (pid_context.pid_output > 0.0)
				{
					pid_context.self_balance_pid_setpoint -= 0.015;                  //Decrease the self_balance_pid_setpoint if the robot is still moving backwards
				}

				if (pid_context.self_balance_pid_setpoint > (MAX_SELF_BALANCE_ADJUST + NATURAL_BALANCE_ANGLE))
				{
					pid_context.self_balance_pid_setpoint = (MAX_SELF_BALANCE_ADJUST + NATURAL_BALANCE_ANGLE);
				}
				else if (pid_context.self_balance_pid_setpoint < (-MAX_SELF_BALANCE_ADJUST + NATURAL_BALANCE_ANGLE))
				{
					pid_context.self_balance_pid_setpoint = (-MAX_SELF_BALANCE_ADJUST + NATURAL_BALANCE_ANGLE);
				}

				LOG_PID("self_balance_pid_setpoint: %f\r\n", pid_context.self_balance_pid_setpoint);
			}

			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Motor pulse calculations
			///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//To compensate for the non-linear behaviour of the stepper motors the folowing calculations are needed to get a linear speed behaviour.
			/*if(pid_output_left > 0.0f)
			{
				pid_output_left = 405 - (1/(pid_output_left + 9)) * 5500;
			}
			else if(pid_output_left < 0.0f)
			{
				pid_output_left = -405 - (1/(pid_output_left - 9)) * 5500;
			}

			if(pid_output_right > 0.0f)
			{
				pid_output_right = 405 - (1/(pid_output_right + 9)) * 5500;
				}
			else if(pid_output_right < 0.0f)
			{
				pid_output_right = -405 - (1/(pid_output_right - 9)) * 5500;
			}*/

			//Calculate the needed pulse time for the left and right stepper motor controllers
			if(pid_output_left > 0.0f)
			{
				//pid_context.motor_left_sps = 50000.0f / ((float)(MAX_PID_OUTPUT - pid_output_left));
				pid_context.motor_left_sps = pid_output_left;
				drv8825_set_direction(motor_left_dev, LEFT_MOTOR_FORWARD);
				drv8825_set_pulse_per_second(motor_left_dev, pid_context.motor_left_sps);
				drv8825_start(motor_left_dev);
			}
			else if(pid_output_left < 0.0f)
			{
				//pid_context.motor_left_sps = 50000.0f / ((float)(MAX_PID_OUTPUT - (-1.0f * pid_output_left)));
				pid_context.motor_left_sps = (-1.0f * pid_output_left);
				drv8825_set_direction(motor_left_dev, LEFT_MOTOR_BACKWARD);
				drv8825_set_pulse_per_second(motor_left_dev, pid_context.motor_left_sps);
				drv8825_start(motor_left_dev);
			}
			else 
			{
				pid_context.motor_left_sps = 0;
				drv8825_stop(motor_left_dev);
			}

			if(pid_output_right > 0.0f)
			{
				//pid_context.motor_right_sps = 50000.0f / ((float)(MAX_PID_OUTPUT - pid_output_right));
				pid_context.motor_right_sps = pid_output_right;
				drv8825_set_direction(motor_right_dev, RIGHT_MOTOR_FORWARD);
				drv8825_set_pulse_per_second(motor_right_dev, pid_context.motor_right_sps);
				drv8825_start(motor_right_dev);
			}
			else if(pid_output_right < 0.0f)
			{
				//pid_context.motor_right_sps = 50000.0f / ((float)(MAX_PID_OUTPUT - (-1.0f * pid_output_right)));
				pid_context.motor_right_sps = (-1.0f * pid_output_right);
				drv8825_set_direction(motor_right_dev, RIGHT_MOTOR_BACKWARD);
				drv8825_set_pulse_per_second(motor_right_dev, pid_context.motor_right_sps);
				drv8825_start(motor_right_dev);
			}
			else
			{
				pid_context.motor_right_sps = 0;
				drv8825_stop(motor_right_dev);
			}

			LOG_PID("pid_output_right: %f\r\n", pid_output_right);
			LOG_PID("pid_output_left: %f\r\n", pid_output_left);
		}

		k_sleep(LOOP_MS);
	}
}