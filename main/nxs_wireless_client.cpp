#include <esp_log.h>
#include "nxs_wireless_client.hpp"

// NXS Wireless:
//        Service UUID: "a5c1c000-cc20-ba91-0c1a-ef3f9e643d79"
// Characteristic
//           Auth UUID: "a5c1cc02-cc20-ba91-0c1a-ef3f9e643d79"
//        Control UUID: "a5c1cc01-cc20-ba91-0c1a-ef3f9e643d79"

#define tag "NXSClient"

const ble_uuid128_t NXSWirelessClient::service = {
    .u	 = {.type = BLE_UUID_TYPE_128},
    .value = {
	   0x79, 0x3d, 0x64, 0x9e, 0x3f, 0xef,
	   0x1a, 0x0c, 0x91, 0xba, 0x20, 0xcc,
	   0x00, 0xc0, 0xc1, 0xa5},
};

const ble_uuid128_t NXSWirelessClient::control_characteristic = {
    .u	 = {.type = BLE_UUID_TYPE_128},
    .value = {
	   0x79, 0x3d, 0x64, 0x9e, 0x3f, 0xef,
	   0x1a, 0x0c, 0x91, 0xba, 0x20, 0xcc,
	   0x01, 0xcc, 0xc1, 0xa5},
};

const ble_uuid128_t NXSWirelessClient::auth_characteristic = {
    .u	 = {.type = BLE_UUID_TYPE_128},
    .value = {
	   0x79, 0x3d, 0x64, 0x9e, 0x3f, 0xef,
	   0x1a, 0x0c, 0x91, 0xba, 0x20, 0xcc,
	   0x02, 0xcc, 0xc1, 0xa5},
};

uint8_t NXSWirelessClient::command_shift_up[8]   = {0x10, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
uint8_t NXSWirelessClient::command_shift_down[8] = {0x11, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


NXSWirelessClient::NXSWirelessClient(const char *peer_address) :
	central(SimpleNimbleCentral::get_instance()) {
	address.type = BLE_ADDR_RANDOM;
	sscanf(peer_address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		  &address.val[0], &address.val[1], &address.val[2],
		  &address.val[3], &address.val[4], &address.val[5]);

	for (int i = 0; i < 6; i++) {
		command_shift_up[2 + i]	 = address.val[i];
		command_shift_down[2 + i] = address.val[i];
	}
}

bool NXSWirelessClient::send(const uint8_t *command, size_t length) {
	return central->write(command, length);
}

bool NXSWirelessClient::connect(const uint8_t *pin) {
	if (!central->connect((const ble_addr_t *)&address)) return false;
	if (!central->find_service((const ble_uuid_t *)&service)) return false;
	if (!central->find_characteristic((const ble_uuid_t *)&auth_characteristic)) return false;
	if (!central->write(pin, 4)) return false;
	if (!central->find_characteristic((const ble_uuid_t *)&control_characteristic)) return false;
	return true;
}

bool NXSWirelessClient::disconnect() {
	return central->disconnect();
}

bool NXSWirelessClient::connect_send_disconnect(const uint8_t *pin, const uint8_t *command, size_t length) {
	if (!connect(pin)) return false;
	if (!send(command, length)) return false;
	return disconnect();
}
