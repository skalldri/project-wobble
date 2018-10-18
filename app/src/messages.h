#pragma once

#include "bt.h"

//
// This file defines all the messages and message queues used by tasks to communicate with each other.
// Tasks are free to define message queues that are used internally. However, all message queues which 
// will be used for inter-task communication must be defined and declared here.
// The data-types that messages will contain must also be declared in this file.
// 

//
// Message queue declarations
//
// Use "extern" variables to help other C files find the message
// queue structures that are declared in messages.c
//
extern struct k_msgq bluetooth_task_queue;
extern struct k_msgq ui_task_queue;

//
// Message definitions
//

// Bluetooth task
typedef struct {
    uint8_t command;
    BT_DEVICE* device;
} BLUETOOTH_TASK_MESSAGE;

typedef enum {
    BLUETOOTH_READY,
    BLUETOOTH_START_SCAN,
    BLUETOOTH_STOP_SCAN,
    BLUETOOTH_PAIR_DEVICE,

    // !!! INSERT NEW COMMANDS ABOVE THIS LINE !!!
    BLUETOOTH_TASK_COMMAND_COUNT
} BLUETOOTH_TASK_COMMAND;

// UI task
typedef struct {
    uint8_t command;
} UI_TASK_MESSAGE;

typedef enum {
    // Menu navigation
    UI_SELECT,
    UI_BACK,
    UI_UP,
    UI_DOWN,

    // Notifications from other tasks
    UI_BT_DEVICE_LIST_UPDATED,

    // !!! INSERT NEW COMMANDS ABOVE THIS LINE !!!
    UI_TASK_COMMAND_COUNT
} UI_TASK_COMMAND;
