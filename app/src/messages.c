#include <zephyr.h>
#include "messages.h"

K_MSGQ_DEFINE(bluetooth_task_queue, sizeof(BLUETOOTH_TASK_MESSAGE), 10, 4);

