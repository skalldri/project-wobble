#pragma once

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <assert.h>

//
// Typedefs
// 

typedef u8_t (*bt_gatt_read_char_value_func_t)(struct bt_conn *conn,
				      struct bt_gatt_subscribe_params *params,
				      const void *data, u16_t length);

typedef struct
{
    struct bt_uuid_16 uuid;
    struct bt_gatt_discover_params discover_params;
    struct bt_gatt_read_params read_params;
    bt_gatt_read_char_value_func_t notify;
} bt_gatt_read_char_value_params;

#pragma pack(push, 1)
typedef struct {
    uint8_t properties;
    uint16_t char_val_handle;
    union {
        uint16_t uuid2;
        uint32_t uuid4;
        uint8_t  uuid16[16];
    } uuid;
} gatt_char_decl_attr;
#pragma pack(pop)
static_assert(sizeof(gatt_char_decl_attr) == 19, "Characteristic Declaration Value must be 19 bytes");

// 
// Public Functions
// 

void hog_parser_task(void *arg1, void *arg2, void *arg3);