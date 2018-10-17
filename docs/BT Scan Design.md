# Bluetooth Scan Design
Zephyr Bluetooth LE scanning is an asynchronous process. The flow works like this:

1. The application calls bt_le_scan_start() and provides a scan callback.
2. The scan callback is called as BT LE devices are discovered.
3. The callback parses the BT LE advertisement data to discover devices which can be paired with. The callback also parses the device name from the advertisement data.
4. The application calls bt_conn_create_le() to pair with the LE device

The connection to the LE device is then established. 

#### UNSURE
The device is requried to store the address of the device that is paired to in flash memory, so that it can be provided to bt_le_set_auto_conn() on the next boot.
#### END UNSURE

## Bluetooth Scan Data Flow

Two Zephyr tasks are responsible for co-ordinating the pairing process:
- UI Task
- Bluetooth Task

To start pairing, the UI Task first posts a BLUETOOTH_START_SCAN message to the BT task message queue. 

The UI Task posts this message when the user activates the "Start Pairing" option in the main menu system. After sending the BLUETOOTH_START_SCAN message, the UI Task changes to the BT Scan state. In this state, the user is locked into a BT Scan UI, which is initially blank. On this UI, if the user presses the "Back" key, the UI task sends a BLUETOOTH_STOP_SCAN message to the Bluetooth task and then returns to the main menu system.

After receiving the BLUETOOTH_START_SCAN message, the Bluetooth task begins searching for BT LE devices. As devices are found, the BT task populates data structures containing the device address, device name, and device type. Only devices with fully populated data structures are considered pairable. Once a data structure is fully populated, a pointer to the data structure is sent to the UI task message queue.

When the UI task receives a Bluetooth device structure, it updates the UI to display the new device. If there are more devices found than can fit on screen, the UI allows the user to scroll up and down the list of devices to select different devices. 

With a device selected, if the user presses the "Select" key, the UI task sends a pointer to the currently selected Bluetooth device structure back to the Bluetooth task in the BLUETOOTH_PAIR_DEVICE message. The UI task should then lock the UI onto the currently selected Bluetooth device, and change state to indicate that pairing is in-progress.

Upon receiving the BLUETOOTH_PAIR_DEVICE message, the Bluetooth task stops scanning for LE devices and begins pairing with the selected device. If pairing succeeds, the Bluetooth task sends a UI_PAIRING_SUCCESSFUL message to the UI task. If pairing fails, it sends a UI_PAIRING_FAILED message to the UI task.

After receiving a BLUETOOTH_PAIR_DEVICE message, the Bluetooth task will not update the contents of any Bluetooth device structures. This allows the UI task to continue rendering device data. Bluetooth device structures will be invalidated and recycled when the Bluetooth task receives the next BLUETOOTH_START_SCAN message.