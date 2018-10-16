#pragma once

#define BT_DISCOVERY_MAX_DEVICES 10

typedef struct {
    uint8_t command;
} BLUETOOTH_TASK_MESSAGE;

enum {
    BLUETOOTH_READY,
    BLUETOOTH_DISCOVERY_COMPLETE,

    // !!! INSERT NEW COMMANDS ABOVE THIS LINE !!!
    BLUETOOTH_TASK_COMMAND_COUNT
} BLUETOOTH_TASK_COMMAND;

void bluetooth_task(void *arg1, void *arg2, void *arg3);