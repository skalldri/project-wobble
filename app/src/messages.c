#include <zephyr.h>
#include "messages.h"

K_MSGQ_DEFINE(bluetooth_task_queue, sizeof(BLUETOOTH_TASK_MESSAGE), 10, 4);

K_MSGQ_DEFINE(ui_task_queue, sizeof(UI_TASK_MESSAGE), 10, 4);
