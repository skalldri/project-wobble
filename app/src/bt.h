#pragma once

#include <bluetooth\bluetooth.h>
#include <bluetooth\uuid.h>

//
// Defines
//
#define BT_MAX_NAME_LEN 30

//
// Typedefs
//

typedef struct {
    char                name[BT_MAX_NAME_LEN];
    bt_addr_le_t        address;
} BT_DEVICE;

//
// Public function definitions
//

void bluetooth_task(void *arg1, void *arg2, void *arg3);