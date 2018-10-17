#include <zephyr.h>
#include <bluetooth\bluetooth.h>
#include <bluetooth\uuid.h>
#include <misc\byteorder.h>
#include "messages.h"
#include "bt.h"

//
// Zephyr Kernel structure definitions
//

//
// Private variable definitions
//


//
// Private function definitions
//

static void bt_ready(int err);
static void start_bt_le_scan();

//
// Public function declarations
//

void bluetooth_task(void *arg1, void *arg2, void *arg3)
{
    int err;

    printf("Starting Bluetooth task\r\n");

	// Initialize the Bluetooth Subsystem
	err = bt_enable(bt_ready);
	if (err) {
		printf("Bluetooth init failed (err %d)\r\n", err);
	}

	BLUETOOTH_TASK_MESSAGE message;

    while(true)
    {
		k_msgq_get(&bluetooth_task_queue, &message, K_FOREVER);

		switch(message.command)
		{
			case BLUETOOTH_READY:
				printf("Bluetooth ready\r\n");
				break;

			case BLUETOOTH_START_SCAN:
				printf("Starting BLE Scan...\r\n");
				start_bt_le_scan();
				break;

			case BLUETOOTH_STOP_SCAN:
				break;

			default:
				__ASSERT(false, "Invalid Bluetooth Task command received");
		}
    }
}

//
// Private function declarations
//

void bt_ready(int err)
{
	if (err) {
		printf("Bluetooth init failed (err %d)\r\n", err);
		return;
	}

	BLUETOOTH_TASK_MESSAGE message;
	message.command = BLUETOOTH_READY;

	while (k_msgq_put(&bluetooth_task_queue, &message, K_NO_WAIT) != 0) 
	{
		// message queue is full: purge old data & try again
		k_msgq_purge(&bluetooth_task_queue);
	}

	printf("Bluetooth initialized\r\n");
}

/*

Steam Controller advertisement looks like this:

[DEVICE]: ff:c1:22:f6:ae:dc (random), AD evt type 0, AD data len 28, RSSI -33
        [AD]: 25 data_len 2                    -> BT_DATA_GAP_APPEARANCE
        [AD]: 1 data_len 1					   -> BT_DATA_FLAGS
        [AD]: 2 data_len 2					   -> BT_DATA_UUID16_SOME	
        [AD]: UUID16 FOUND - 6162			   -> BT_UUID_HIDS
        [AD]: 9 data_len 15					   -> BT_DATA_NAME_COMPLETE
        [AD]: NAME FOUND - SteamController ▒D
*/

static bool bt_data_parse_callback(struct bt_data* data, void* user_data)
{
	BT_DEVICE *device = user_data;

	printf("\t[AD]: %u data_len %u\r\n", data->type, data->data_len);

	switch (data->type) 
	{
		case BT_DATA_UUID16_SOME:
		case BT_DATA_UUID16_ALL:
			if (data->data_len % sizeof(u16_t) != 0) 
			{
				printf("AD malformed\r\n");
				return true;
			}

			for (int i = 0; i < data->data_len; i += sizeof(u16_t)) 
			{
				struct bt_uuid *uuid;
				u16_t u16;
				int err;

				memcpy(&u16, &data->data[i], sizeof(u16));
				u16 = sys_le16_to_cpu(u16);
				uuid = BT_UUID_DECLARE_16(u16);

				printf("\t[AD]: UUID16 FOUND - %u\r\n", u16);
			}
			break;

		case BT_DATA_NAME_COMPLETE:
		case BT_DATA_NAME_SHORTENED:	
			memcpy(device->name, data->data, min(data->data_len, BT_MAX_NAME_LEN - 1));
			printf("\t[AD]: NAME FOUND - %s\r\n", device->name);
			break;		
	}

	return true;
}

static void bt_le_scan_callback(const bt_addr_le_t* addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple* buf)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	BT_DEVICE device;

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	printf("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\r\n",
	       addr_str, adv_type, buf->len, rssi);

	/* We're only interested in connectable events */
	if (adv_type == BT_LE_ADV_IND || adv_type == BT_LE_ADV_DIRECT_IND) 
	{
		bt_data_parse(buf, bt_data_parse_callback, (void *)&device);
	}
}

void start_bt_le_scan()
{
	int err;
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, bt_le_scan_callback);

	if (err == 0)
	{
		printf("Bluetooth device discovery started\r\n");
	}
	else
	{
		printf("Bluetooth device discovery failed to start: err %d\r\n", err);
	}
}
