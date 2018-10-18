// System includes
#include <zephyr.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <misc/byteorder.h>
#include <stdio.h>

// Application includes
#include "messages.h"
#include "hog_parser.h"
#include "bt.h"

//
// Private function definitions
//

static void bt_ready(int err);
static void start_bt_le_scan();
static BT_DEVICE* get_empty_device_list_slot();
static void initialize_device_list();
static bool is_address_in_device_list(const bt_addr_le_t* addr);
static void pair_device(BT_DEVICE* device);
static void device_connected(struct bt_conn *conn, u8_t err);
static void device_disconnected(struct bt_conn *conn, u8_t reason);
static u8_t gatt_notify_cb(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length);
static u8_t gatt_discover_cb(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params);
static u8_t gatt_read_cb(struct bt_conn *conn, u8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, u16_t length);

//
// Public variable declarations
//
BT_DEVICE_LIST g_device_list;

//
// Private variable declarations
//
static struct bt_conn_cb bt_conn_callbacks = {
		.connected = device_connected,
		.disconnected = device_disconnected,
};

// TODO: robustify BT connection management, potentially allow multiple connections?
static struct bt_conn *default_conn;

static struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
static struct bt_gatt_read_params read_params;

#define HOGP_TASK_STACK_SIZE (2048)
K_THREAD_STACK_DEFINE(hogp_task_stack, HOGP_TASK_STACK_SIZE);
static struct k_thread hogp_data;

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
		return;
	}

	bt_conn_cb_register(&bt_conn_callbacks);

	initialize_device_list();

	BLUETOOTH_TASK_MESSAGE message;

    while(true)
    {
		k_msgq_get(&bluetooth_task_queue, &message, K_FOREVER);

		switch((BLUETOOTH_TASK_COMMAND) message.command)
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

			case BLUETOOTH_PAIR_DEVICE:
				pair_device(message.device);
				break;

			default:
				__ASSERT(false, "Invalid Bluetooth Task command received");
		}
    }
}

// Convenience function to take the global device list lock
void take_device_list_lock()
{
	k_mutex_lock(&g_device_list.lock, K_FOREVER);
}

// Convenience function to release the global device list lock
void release_device_list_lock()
{
	k_mutex_unlock(&g_device_list.lock);
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

	//printf("\t[AD]: %u data_len %u\r\n", data->type, data->data_len);

	switch (data->type) 
	{
		case BT_DATA_UUID16_SOME:
		case BT_DATA_UUID16_ALL:
			if (data->data_len % sizeof(u16_t) != 0) 
			{
				//printf("AD malformed\r\n");
				return true;
			}

			for (int i = 0; i < data->data_len; i += sizeof(u16_t)) 
			{
				struct bt_uuid *uuid;
				u16_t u16;

				memcpy(&u16, &data->data[i], sizeof(u16));
				u16 = sys_le16_to_cpu(u16);
				uuid = BT_UUID_DECLARE_16(u16);

				//printf("\t[AD]: UUID16 FOUND - %u\r\n", u16);

				if (bt_uuid_cmp(uuid, BT_UUID_HIDS) == 0)
				{
					device->is_hid_device = true;
				}
			}
			break;

		case BT_DATA_NAME_COMPLETE:
		case BT_DATA_NAME_SHORTENED:	
			memcpy(device->name, data->data, min(data->data_len, BT_MAX_NAME_LEN - 1));
			//printf("\t[AD]: NAME FOUND - %s\r\n", device->name);
			break;		
	}

	return true;
}

static void bt_le_scan_callback(const bt_addr_le_t* addr, int8_t rssi, uint8_t adv_type, struct net_buf_simple* buf)
{
	BT_DEVICE device = {0};

	// We're only interested in connectable events
	if (adv_type == BT_LE_ADV_IND || adv_type == BT_LE_ADV_DIRECT_IND)
	{
		memcpy(&device.address, addr, sizeof(bt_addr_le_t));

		bt_data_parse(buf, bt_data_parse_callback, (void *)&device);

		// If we detected that this is a HID device and it has a name,
		// we will add it to the list of pair-able devices
		if (device.is_hid_device && device.name[0] != '\0')
		{		   
			// Must take the device list lock before reading or writing
			// Must hold the lock while making modifications (like populating a cell)
			take_device_list_lock();
			{
				// Do nothing if this is a duplicate advertisement
				if (!is_address_in_device_list(addr))
				{
					// Copy our local device data into a slot in the global device structure
					BT_DEVICE* empty_slot = get_empty_device_list_slot();

					if (empty_slot != NULL)
					{
						// Mark the device as valid and copy to the destination buffer
						device.is_valid = true;
						memcpy(empty_slot, &device, sizeof(BT_DEVICE));
					}
					else
					{
						printf("WARNING: bluetooth device list is full. Cannot add new device\r\n");
					}
				}
			}
			release_device_list_lock();

			// Announce that the discovered device list has changed
			UI_TASK_MESSAGE message;
			message.command = UI_BT_DEVICE_LIST_UPDATED;

			while (k_msgq_put(&ui_task_queue, &message, K_NO_WAIT) != 0)
			{
				// message queue is full: purge old data & try again
				k_msgq_purge(&ui_task_queue);
			}
		}
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

static void pair_device(BT_DEVICE* device)
{
	// Stop BT LE scanning
	int status = bt_le_scan_stop();

	if (status == -EALREADY)
	{
		printf("WARN: Odd state. Attempted to pair to device while not in BT LE scan mode\r\n");
	}
	else if (status != 0)
	{
		printf("WARN: Unknown error while stopping BT LE Scan. Err = %d\r\n", status);
	}

	default_conn = bt_conn_create_le(&device->address, BT_LE_CONN_PARAM_DEFAULT);
}

static void device_connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printf("Failed to connect to %s (%u)\r\n", addr, err);
		return;
	}

	if (conn != default_conn) 
	{
		printf("WARNING: connected to device, but bt_conn does not match global connection\r\n");
		return;
	}

	printf("Connected: %s\r\n", addr);

	// Create the HOG Parser task to go and perform the parsing of this device's
	// HID-over-GATT profile (if it exists...)
	k_thread_create(&hogp_data, 						
					hogp_task_stack,						// Data buffer for thread stack
					HOGP_TASK_STACK_SIZE,			    	// Thread stack size 
					(k_thread_entry_t) hog_parser_task,   	// Thread function
					default_conn, 							// Parameter 1
					NULL, 									// Parameter 2
					NULL, 									// Parameter 3
					K_PRIO_COOP(7),							// Thread priority 
					0, 										// Thread options
					K_NO_WAIT);								// Scheduling delay

	// Discover HID services on the connected device
	/*memcpy(&uuid, BT_UUID_HIDS, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.func = gatt_discover_cb;
		discover_params.start_handle = 0x0001;
		discover_params.end_handle = 0xffff;
		discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	int status = bt_gatt_discover(default_conn, &discover_params);
	if (status) {
		printk("Discover failed (err %d)\r\n", status);
	}*/
}

static void device_disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != default_conn) {
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printf("Disconnected: %s (reason %u)\r\n", addr, reason);

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

/*
static u8_t gatt_notify_cb(struct bt_conn *conn,
			   struct bt_gatt_subscribe_params *params,
			   const void *data, u16_t length)
{
	if (!data) {
		printk("[UNSUBSCRIBED]\r\n");
		params->value_handle = 0;
		return BT_GATT_ITER_STOP;
	}

	printk("[NOTIFICATION] data %p length %u\r\n", data, length);

	return BT_GATT_ITER_CONTINUE;
}
*/
/*
static u8_t gatt_read_cb(struct bt_conn *conn, u8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, u16_t length)
{
	printf("Read complete: err %u length %u\n", err, length);

	if (!data) {
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	gatt_char_decl_attr* attr = (gatt_char_decl_attr*) data;

	read_params()

	bt_gatt_read(conn, &read_params);

	return BT_GATT_ITER_CONTINUE;
}*/

static u8_t gatt_discover_cb(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	int err;

	if (!attr) {
		printk("Discover complete\r\n");
		memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

	printk("[ATTRIBUTE] handle %u\r\n", attr->handle);

	if (bt_uuid_cmp(discover_params.uuid, BT_UUID_HIDS) == 0)
	{
		// First level of parsing: we found HID Service.
		// Lets look for the BT_UUID_HIDS_REPORT_MAP characteristic
		// This characteristic contains the HID report descriptor
		memcpy(&uuid, BT_UUID_HIDS_REPORT_MAP, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 1;
		discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\r\n", err);
		}
	} 
	else if (bt_uuid_cmp(discover_params.uuid, BT_UUID_HIDS_REPORT_MAP) == 0) 
	{
		// Second level of parsing: we found the BT_UUID_HIDS_REPORT_MAP,
		// characteristic, which contains the HID report descriptor
		/*read_params.func = gatt_read_cb;
		read_params.handle_count = 1;
		read_params.single.handle = attr->handle;
		read_params.single.offset = 0;
		
		bt_gatt_read(conn, &read_params);*/

		/*memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
		discover_params.uuid = &uuid.uuid;
		discover_params.start_handle = attr->handle + 2;
		discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		subscribe_params.value_handle = attr->handle + 1;

		err = bt_gatt_discover(conn, &discover_params);
		if (err) {
			printk("Discover failed (err %d)\n", err);
		}*/

	}/* else {
		subscribe_params.notify = notify_func;
		subscribe_params.value = BT_GATT_CCC_NOTIFY;
		subscribe_params.ccc_handle = attr->handle;

		err = bt_gatt_subscribe(conn, &subscribe_params);
		if (err && err != -EALREADY) {
			printk("Subscribe failed (err %d)\n", err);
		} else {
			printk("[SUBSCRIBED]\n");
		}

		return BT_GATT_ITER_STOP;
	}*/

	return BT_GATT_ITER_STOP;
}


// Check to see if a bluetooth address is already associated with a device in the 
// global BT device list
bool is_address_in_device_list(const bt_addr_le_t* addr)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(g_device_list.devices); i++)
	{
		if (g_device_list.devices[i].is_valid)
		{
			if (bt_addr_le_cmp(&g_device_list.devices[i].address, addr) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

// Returns a pointer to the first empty BT_DEVICE structure in the global
// device list.
// The global device list lock must be taken before calling this function
BT_DEVICE* get_empty_device_list_slot()
{
	BT_DEVICE* empty_slot = NULL;

	for (uint32_t i = 0; i < ARRAY_SIZE(g_device_list.devices); i++)
	{
		if (g_device_list.devices[i].is_valid == false)
		{
			empty_slot = &g_device_list.devices[i];
			break;
		}
	}

	return empty_slot;
}

void initialize_device_list()
{
	memset(&g_device_list.devices, 0, ARRAY_SIZE(g_device_list.devices));
	k_mutex_init(&g_device_list.lock);
}