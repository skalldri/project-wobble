#include <zephyr.h>
#include <ring_buffer.h>

#include "messages.h"
#include "hog_parser.h"

/** Summary:
 *		The HID-over-GATT (HOG) parser task is designed to parse the GATT Profile information 
 *		provided by a bluetooth device. 
 
 *
 *		The Zephyr BT LE GATT API's are designed to run asynchronously. To perform the HOG profile parsing, the host must
 *		execute a set of operations in order, following a state machine. 
 * 
 * 
 *
 */

//
// Typedefs
//

// This enum defines the events that the HID-over-GATT parser task will respond to
typedef enum {
	GATT_PARSE_BEGIN,			// Start the parsing sequence
	GATT_DISCOVER_COMPLETE,		// The paser has finished a GATT Discovery operation
    GATT_READ_COMPLETE,    	    // The parser has finished a GATT Read operation

    // !!! INSERT NEW MESSAGES ABOVE THIS LINE !!!
    HOG_PARSER_MESSAGE_COUNT
} HOG_PARSER_COMMAND;

typedef struct {
    uint8_t command;
} HOG_PARSER_MESSAGE;

// TODO: maybe get a state machine library to build this out?
typedef enum {

	HOGP_STATE_INIT,
	HOGP_STATE_HID_SVC_DISCOVER,
	HOGP_STATE_HID_REPORT_MAP_CHAR_DISCOVER,
	HOGP_STATE_HID_REPORT_MAP_CHAR_READ,

	// TODO: more states

	HOGP_STATE_COMPLETE,							// The last state in a successful HOG parse
	HOGP_STATE_INVALID,								// This state can be entered from any other state to indicate an error has occurred

	// !!! INSERT NEW STATES ABOVE THIS LINE !!!
	HOG_PARSER_STATE_COUNT
} HOG_PARSER_STATE;

#define GATT_READ_BUFFER_SIZE 512
typedef struct {
	HOG_PARSER_STATE current_state;		// The current state
	HOG_PARSER_STATE next_state;		// The state should we transition to next
	bool ready_to_advance;				// Is the current state ready to advance to the next state?

	// Bluetooth connection info
	struct bt_conn* conn;

	// GATT Discovery Context
	int discover_status;
	struct bt_gatt_attr attr;

	// GATT Read Context
	int read_status;
	// TODO: update to latest version of Zephyr
	// 		 and convert this into a byte ring buffer
	uint32_t read_buffer_index;
	uint8_t read_buffer[GATT_READ_BUFFER_SIZE];

} HOG_PARSER_CONTEXT;

#define HID_REPORT_DESCRIPTOR_BUFFER_SIZE 512
uint32_t s_hid_report_descriptor_size;
uint8_t s_hid_report_descriptor[HID_REPORT_DESCRIPTOR_BUFFER_SIZE];

//
// Private variable definitions
//

static struct bt_uuid_16                uuid = BT_UUID_INIT_16(0);
static struct bt_gatt_discover_params   discover_params;
static struct bt_gatt_read_params       read_params;

K_MSGQ_DEFINE(hog_parser_task_queue, sizeof(HOG_PARSER_MESSAGE), 10, 4);

static HOG_PARSER_CONTEXT s_context;

//
// Private function declarations
//

static u8_t gatt_discover_cb(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params);

static u8_t gatt_read_cb(struct bt_conn *conn, u8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, u16_t length);

static void hog_state_machine(HOG_PARSER_MESSAGE* message);
static void hogp_state_init(HOG_PARSER_MESSAGE* message);
static void hogp_state_hid_svc_discover(HOG_PARSER_MESSAGE* message);
static void hogp_state_hid_report_map_char_discover(HOG_PARSER_MESSAGE* message);
static void hogp_state_hid_report_map_char_read(HOG_PARSER_MESSAGE* message);

static void hog_parser_send_event(HOG_PARSER_COMMAND event);
static void hogp_change_state(HOG_PARSER_STATE state);

//
// Public function definitions
//

void hog_parser_task(void *bt_conn, void *arg2, void *arg3)
{
	bool parsing_in_progress = true;
	HOG_PARSER_MESSAGE message;
	int status;

	// Setup the task's context block
	// TODO: functionize
	s_context.current_state = HOGP_STATE_INIT;
	s_context.ready_to_advance = false;
	s_context.next_state = HOGP_STATE_INVALID;

	s_context.conn = (struct bt_conn*) bt_conn;

	// Kick start the state machine
	hog_parser_send_event(GATT_PARSE_BEGIN);

	// TODO: we may not want this task to exit
	while(parsing_in_progress)
	{
		status = k_msgq_get(&hog_parser_task_queue, &message, K_FOREVER);

		printf("HOGP EVENT: %d\r\n", message.command);

		// Run the state machine parser now that we've completed an action
		// Always run the state machine at least once. If the ready_to_advance flag is
		// set, continue to run the state machine until it is no longer set.
		do
		{
			hog_state_machine(&message);
		} while (s_context.ready_to_advance);
	}
}

static void hog_state_machine(HOG_PARSER_MESSAGE* message)
{
	// Advance to the next state if needed.
	if (s_context.ready_to_advance)
	{
		printf("HOGP State machine advancing states: %d -> %d\r\n", s_context.current_state, s_context.next_state);
		s_context.current_state = s_context.next_state;
		s_context.next_state = HOGP_STATE_INVALID;
		s_context.ready_to_advance = false;

		// Change the message to GATT_PARSE_BEGIN to indicate that
		// this is the first entry into the new state
		message->command = GATT_PARSE_BEGIN;
	}

	switch (s_context.current_state)
	{
		// The state machine has just been initialized. We should start parsing the GATT Profile.
		// Send a service discovery request for the HID Service.
		case HOGP_STATE_INIT:
			hogp_state_init(message);
			break;

		case HOGP_STATE_HID_SVC_DISCOVER:
			hogp_state_hid_svc_discover(message);
			break;

		case HOGP_STATE_HID_REPORT_MAP_CHAR_DISCOVER:
			hogp_state_hid_report_map_char_discover(message);
			break;

		case HOGP_STATE_HID_REPORT_MAP_CHAR_READ:
			hogp_state_hid_report_map_char_read(message);
			break;

		case HOGP_STATE_COMPLETE:
			__ASSERT(false, "HOG PARSE STATE MACHINE ERROR!!!!");
			break;
	}
}

static void hogp_state_init(HOG_PARSER_MESSAGE* message)
{
	switch((HOG_PARSER_COMMAND) message->command)
	{
		case GATT_PARSE_BEGIN:
			// TODO: reset all state machine parameters
			hogp_change_state(HOGP_STATE_HID_SVC_DISCOVER);
			break;

		// Invalid state transition
		default:
			printf("INVALID COMMAND %d\r\n", message->command);
			__ASSERT(false, "INVALID STATE TRANSITION");
			break;
	}
}

static void hogp_state_hid_svc_discover(HOG_PARSER_MESSAGE* message)
{
	switch((HOG_PARSER_COMMAND) message->command)
	{
		case GATT_PARSE_BEGIN:
			// Discover the HID Service on the connected device
			memcpy(&uuid, BT_UUID_HIDS, sizeof(uuid));
			discover_params.uuid = &uuid.uuid;
			discover_params.func = gatt_discover_cb;
			discover_params.start_handle = 0x0001;
			discover_params.end_handle = 0xffff;
			discover_params.type = BT_GATT_DISCOVER_PRIMARY;

			int status = bt_gatt_discover(s_context.conn, &discover_params);
			if (status) {
				printf("Discover failed (err %d)\r\n", status);
			}
			break;

		case GATT_DISCOVER_COMPLETE:
			// If we found the HID Service, advance to the next state
			// Else, error out of the state machine
			if (s_context.discover_status == 0)
			{
				hogp_change_state(HOGP_STATE_HID_REPORT_MAP_CHAR_DISCOVER);
			}
			else
			{
				hogp_change_state(HOGP_STATE_INVALID);
			}
			break;

		// Invalid state transition
		default:
			__ASSERT(false, "INVALID STATE TRANSITION");
			break;
	}
}

static void hogp_state_hid_report_map_char_discover(HOG_PARSER_MESSAGE* message)
{
	switch((HOG_PARSER_COMMAND) message->command)
	{
		case GATT_PARSE_BEGIN:
			printf("Discovering HID Report Map Characteristic\r\n");
			// Discover the HID Report Map (Basically the HID Report Descriptor)
			// on the connected device
			memcpy(&uuid, BT_UUID_HIDS_REPORT_MAP, sizeof(uuid));
			discover_params.uuid = &uuid.uuid;
			discover_params.func = gatt_discover_cb;

			// Search handle IDs in range [ (HID Service Handle + 1), (MAX_HANDLE) ]
			discover_params.start_handle = s_context.attr.handle + 1;
			discover_params.end_handle = 0xffff;
			discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

			int status = bt_gatt_discover(s_context.conn, &discover_params);
			if (status) {
				printf("Discover failed (err %d)\r\n", status);
			}
			break;

		case GATT_DISCOVER_COMPLETE:
			// If we found the Characteristic, advance to the read state
			// Else, error out of the state machine
			if (s_context.discover_status == 0)
			{
				hogp_change_state(HOGP_STATE_HID_REPORT_MAP_CHAR_READ);
			}
			else
			{
				hogp_change_state(HOGP_STATE_INVALID);
			}
			break;

		// Invalid state transition
		default:
			__ASSERT(false, "INVALID STATE TRANSITION");
			break;
	}
}

static void hogp_state_hid_report_map_char_read(HOG_PARSER_MESSAGE* message)
{
	static int read_count = 0;
	int status;

	switch((HOG_PARSER_COMMAND) message->command)
	{
		case GATT_PARSE_BEGIN:
			printf("Reading HID Report Map Characteristic Declaration Attr\r\n");
			// Reset our local read count, in case we've been called twice
			read_count = 0;

			// Read the HID Report Map GATT Characteristic Declaration Attribute
			read_params.func = gatt_read_cb;
			read_params.handle_count = 1;
			read_params.single.handle = s_context.attr.handle;
			read_params.single.offset = 0;

			// Reset GATT Read context
			s_context.read_buffer_index = 0;
			memset(s_context.read_buffer, 0, GATT_READ_BUFFER_SIZE);

			status = bt_gatt_read(s_context.conn, &read_params);
			if (status) 
			{
				printf("Discover failed (err %d)\r\n", status);
			}
			break;

		case GATT_READ_COMPLETE:
			if (s_context.read_status == 0)
			{
				// First read should be the GATT Characteristic Declaration Attribute
				if (read_count == 0)
				{
					read_count++;

					// TODO: allow any of the 3 sizes of Char Decl Attrs (5, 7, 19 bytes)
					if (s_context.read_buffer_index != 5)
					{
						printf("ERROR: Invalid Char Decl Attr size %u\r\n", s_context.read_buffer_index);
						hogp_change_state(HOGP_STATE_INVALID);
						return;
					}

					gatt_char_decl_attr* attribute = (gatt_char_decl_attr*) s_context.read_buffer;

					printf("Reading HID Report Map Characteristic Value Attr\r\n");
					// Read the HID Report Map GATT Characteristic Declaration Attribute
					read_params.func = gatt_read_cb;
					read_params.handle_count = 1;
					read_params.single.handle = attribute->char_val_handle;
					read_params.single.offset = 0;

					// Reset GATT Read context
					s_context.read_buffer_index = 0;
					memset(s_context.read_buffer, 0, GATT_READ_BUFFER_SIZE);

					status = bt_gatt_read(s_context.conn, &read_params);
					if (status) 
					{
						printf("Discover failed (err %d)\r\n", status);
					}
				}
				// Second read, should be the GATT Characteristic Value itself
				else
				{
					printf("GOT HID REPORT DESCRIPTOR\r\n");
					memcpy(s_hid_report_descriptor, s_context.read_buffer, min(s_context.read_buffer_index, HID_REPORT_DESCRIPTOR_BUFFER_SIZE));
					s_hid_report_descriptor_size = s_context.read_buffer_index;

					for (uint32_t i = 0; i < s_hid_report_descriptor_size; i++)
					{
						printf("0x%x\r\n", ((uint8_t*) s_hid_report_descriptor)[i]);
					}
				}
			}
			else
			{
				hogp_change_state(HOGP_STATE_INVALID);
			}
			break;

		// Invalid state transition
		default:
			__ASSERT(false, "INVALID STATE TRANSITION");
			break;
	}
}

static void hog_parser_send_event(HOG_PARSER_COMMAND event)
{
	HOG_PARSER_MESSAGE message;
	message.command = event;

	while (k_msgq_put(&hog_parser_task_queue, &message, K_NO_WAIT) != 0) 
	{
		// message queue is full: purge old data & try again
		k_msgq_purge(&hog_parser_task_queue);
	}
}

static void hogp_change_state(HOG_PARSER_STATE state)
{
	printf("Change state to %d\r\n", state);
	s_context.next_state = state;
	s_context.ready_to_advance = true;
}

static u8_t gatt_discover_cb(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params)
{
	if (attr)
	{
		// We found the requested attribute. Copy it into s_context, and emit a GATT_DISCOVER_COMPLETE event
		memcpy(&s_context.attr, attr, sizeof(struct bt_gatt_attr));
		s_context.discover_status = 0;
	}
	else
	{
		s_context.discover_status = -EEXIST;
	}

	hog_parser_send_event(GATT_DISCOVER_COMPLETE);

	return BT_GATT_ITER_STOP;
}

static u8_t gatt_read_cb(struct bt_conn *conn, u8_t err,
			 struct bt_gatt_read_params *params,
			 const void *data, u16_t length)
{
	printf("Read complete: err %u length %u\r\n", err, length);

	// No more data to read. Mark the read as complete and emit our signal
	// to the HOGP task
	if (!data) 
	{	
		memset(params, 0, sizeof(*params));
		s_context.read_status = err;
		hog_parser_send_event(GATT_READ_COMPLETE);

		return BT_GATT_ITER_STOP;
	}

	// Push this data into the GATT Read ring buffer

	// If we're out of space in the read buffer, abort. Set read_status to an error
	// and signal the task.
	if (s_context.read_buffer_index >= GATT_READ_BUFFER_SIZE)
	{
		printf("ERROR: gatt_read requires larger buffer than GATT_READ_BUFFER_SIZE\r\n");
		s_context.read_status = -ENOMEM;
		hog_parser_send_event(GATT_READ_COMPLETE);

		return BT_GATT_ITER_STOP;
	}

	memcpy(&s_context.read_buffer[s_context.read_buffer_index], 
		   (uint8_t*) data, 
		   min(length, GATT_READ_BUFFER_SIZE));

	for (uint32_t i = 0; i < length; i++)
	{
		printf("0x%x\r\n", ((uint8_t*) data)[i]);
	}

	s_context.read_buffer_index += min(length, GATT_READ_BUFFER_SIZE);

	return BT_GATT_ITER_CONTINUE;
}

/** @brief GATT Read Attribute Value
 *
 *  This procedure performs a series of asynchronous operations to discover and read
 *  GATT Characteristic Values.
 * 
 *  First, the procedure performs GATT Discovery to determine if the indicated GATT Service is present
 *  on the device. Next, it performs GATT Discovery again to determine if the indicated GATT Characteristic is present 
 *  in the service.
 * 
 *  Once the Attribute is located, the procedure reads the GATT Characteristic Declaration Attribute to extract the 
 *  GATT Handle for the Characteristic Value Attribute. This is required by the Bluetooth spec, rather than just assuming
 *  that the Handle for the Characteristic Value Attribute will simply be the next handle.
 *
 *  Finally, after reading the Value from the Characteristic, the user callback is invoked. The data passed to the callback
 *  is only valid within the callback scope. The user callback should copy data from the buffer if it is needed later.
 * 
 *  Note: This procedure is asynchronous therefore the parameters need to
 *  remains valid while it is active.
 *
 *  @param conn Connection object.
 *  @param params Subscribe parameters.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
/*
u8_t gatt_read_char_value(struct bt_conn* conn, struct bt_uuid* service_uuid, )
{
    // TODO: implement a lock to ensure we can only have one outstanding attribute read at a time
    // Top Level -> Discovery CB 1 -> Discovery CB 2 -> GATT Read CB 1 -> GATT Read CB2

    // Discover HID services on the connected device
	memcpy(&uuid, BT_UUID_HIDS, sizeof(uuid));
		discover_service_params.uuid = service_uuid;
		discover_service_params.func = gatt_discover_service_cb;
		discover_service_params.start_handle = 0x0001;
		discover_service_params.end_handle = 0xffff;
		discover_service_params.type = BT_GATT_DISCOVER_PRIMARY;

	int status = bt_gatt_discover(conn, &discover_service_params);
	if (status) {
		printf("Discover failed (err %d)\r\n", status);
	}
}
*/