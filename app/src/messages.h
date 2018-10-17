#pragma once

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

//
// Message definitions
//
typedef struct {
    uint8_t command;
} BLUETOOTH_TASK_MESSAGE;

enum {
    BLUETOOTH_READY,
    BLUETOOTH_START_SCAN,
    BLUETOOTH_STOP_SCAN,

    // !!! INSERT NEW COMMANDS ABOVE THIS LINE !!!
    BLUETOOTH_TASK_COMMAND_COUNT
} BLUETOOTH_TASK_COMMAND;