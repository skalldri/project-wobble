#pragma once

#include <bluetooth\bluetooth.h>
#include <bluetooth\uuid.h>

//
// Defines
//
#define BT_MAX_NAME_LEN 30
#define BT_MAX_SCANNED_DEVICES 20

//
// Typedefs
//

typedef struct {
    char                name[BT_MAX_NAME_LEN];
    bt_addr_le_t        address;
    bool                is_hid_device;
    bool                is_valid;
} BT_DEVICE;

// TODO: An array may not be the right data structure for this, but it works for now.
//       Investigate using one of the kernel data structures for this list
// TODO: An "announce updated" message might not be the best way to handle passing device
//       info arround. Consider re-organizing at a later date
typedef struct {

    BT_DEVICE devices[BT_MAX_SCANNED_DEVICES];
    struct k_mutex lock;
} BT_DEVICE_LIST;

//
// Public variables
//
extern BT_DEVICE_LIST g_device_list;

//
// Public function definitions
//

void bluetooth_task(void *arg1, void *arg2, void *arg3);
void take_device_list_lock();
void release_device_list_lock();