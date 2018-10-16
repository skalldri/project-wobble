#include <zephyr.h>
#include <bluetooth\bluetooth.h>
#include "bt.h"

//
// Zephyr Kernel structure definitions
//

K_MSGQ_DEFINE(bluetooth_task_queue, sizeof(BLUETOOTH_TASK_MESSAGE), 10, 4);

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

    printk("Starting Bluetooth task\n");

	// Initialize the Bluetooth Subsystem
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	BLUETOOTH_TASK_MESSAGE message;

    while(true)
    {
		k_msgq_get(&bluetooth_task_queue, &message, K_FOREVER);

		switch(message.command)
		{
			case BLUETOOTH_READY:
				start_bt_le_scan();
				break;

			case BLUETOOTH_DISCOVERY_COMPLETE:
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
		printk("Bluetooth init failed (err %d)\r\n", err);
		return;
	}

	BLUETOOTH_TASK_MESSAGE message;
	message.command = BLUETOOTH_READY;

	while (k_msgq_put(&bluetooth_task_queue, &message, K_NO_WAIT) != 0) 
	{
		// message queue is full: purge old data & try again
		k_msgq_purge(&bluetooth_task_queue);
	}

	printk("Bluetooth initialized\n");
}

static bool bt_data_parse_callback(struct bt_data* data, void* user_data)
{
	//bt_addr_le_t *addr = user_data;
	//int i;

	printk("\t[AD]: %u data_len %u\r\n", data->type, data->data_len);

	switch (data->type) {
	/*case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(u16_t) != 0) {
			printk("AD malformed\n");
			return true;
		}

		for (i = 0; i < data->data_len; i += sizeof(u16_t)) {
			struct bt_uuid *uuid;
			u16_t u16;
			int err;

			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(uuid, BT_UUID_HRS)) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err) {
				printk("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			default_conn = bt_conn_create_le(addr,
							 BT_LE_CONN_PARAM_DEFAULT);
			return false;
		}
		break;
		*/
	case BT_DATA_NAME_COMPLETE:
	case BT_DATA_NAME_SHORTENED:
	{	
		#define NAME_LEN 30

		char name[NAME_LEN];
		memcpy(name, data->data, min(data->data_len, NAME_LEN - 1));
		printk("\t[AD]: NAME FOUND - %s\r\n", name);
		break;
	}
		
	}

	return true;
}

static void bt_le_scan_callback(const bt_addr_le_t* addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple* buf)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\r\n",
	       dev, adv_type, buf->len, rssi);

	/* We're only interested in connectable events */
	if (adv_type == BT_LE_ADV_IND || adv_type == BT_LE_ADV_DIRECT_IND) {
		bt_data_parse(buf, bt_data_parse_callback, (void *)addr);
	}
}

void start_bt_le_scan()
{
	int err;
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, bt_le_scan_callback);

	if (err == 0)
	{
		printk("Bluetooth device discovery started\r\n");
	}
	else
	{
		printk("Bluetooth device discovery failed to start: err %d\r\n", err);
	}
}
