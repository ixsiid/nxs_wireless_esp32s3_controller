#include <esp_log.h>
#include "nxs_wireless_client.hpp"

// SwitchBot Bot:
//        Service UUID: "a5c1c000-cc20-ba91-0c1a-ef3f9e643d79"
// Characteristic
//     Auth UUID: "a5c1cc02-cc20-ba91-0c1a-ef3f9e643d79"
//  Control UUID: "a5c1cc01-cc20-ba91-0c1a-ef3f9e643d79"

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

NXSWirelessClient::NXSWirelessClient(const char *peer_address) {
	address.type = BLE_ADDR_RANDOM;
	sscanf(peer_address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		  &address.val[0], &address.val[1], &address.val[2],
		  &address.val[3], &address.val[4], &address.val[5]);

	for (int i = 0; i < 6; i++) {
		command_shift_up[2 + i]	 = address.val[i];
		command_shift_down[2 + i] = address.val[i];
	}

	central = new NimbleCentral();
	int rc  = central->start("SBClient");
	assert(rc == 0);
}

typedef struct {
	const ble_addr_t *address;
	NimbleCentral *central;
	const ble_uuid_t *service;
	const ble_uuid_t *characteristic;
	const uint8_t *command;
	size_t length;
	NimbleCallback callback;
	NimbleCallback connecting;
	NimbleCallback writing;
} callback_args_t;

int NXSWirelessClient::send(const uint8_t *command, size_t length) {
	NimbleCallback callback = [](uint16_t, NimbleCallbackReason) {
		return 0;
	};

	central->write((const ble_uuid_t *)&service, (const ble_uuid_t *)&control_characteristic,
				command, length, 10000,
				callback);

	return 0;
}

bool NXSWirelessClient::connect(const uint8_t *pin) {
	static callback_args_t *args = nullptr;

	if (args != nullptr) {
		ESP_LOGI(tag, "busy");
		return 1;
	}

	args				 = new callback_args_t();
	args->address		 = &address;
	args->central		 = central;
	args->service		 = (const ble_uuid_t *)&service;
	args->characteristic = (const ble_uuid_t *)&auth_characteristic;
	args->command		 = pin;
	args->length		 = 4;

	ESP_LOGI(tag, "start connect");

	args->callback = [](uint16_t handle, NimbleCallbackReason reason) {
		switch (reason) {
			case NimbleCallbackReason::SUCCESS:
				ESP_LOGI(tag, "command write success");
				delete args;
				args = nullptr;
				break;
			case NimbleCallbackReason::CONNECTION_START:
			case NimbleCallbackReason::CHARACTERISTIC_WRITE_FAILED:
			case NimbleCallbackReason::CHARACTERISTIC_FIND_FAILED:
			case NimbleCallbackReason::SERVICE_FIND_FAILED:
			case NimbleCallbackReason::STOP_CANCEL_FAILED:
			case NimbleCallbackReason::CONNECTION_FAILED:
			case NimbleCallbackReason::OTHER_FAILED:
				ESP_LOGI(tag, "command failed");
				delete args;
				args = nullptr;
				break;
			case NimbleCallbackReason::CONNECTION_ESTABLISHED:
				ESP_LOGI(tag, "connected, start write");
				args->central->write(args->service, args->characteristic,
								 args->command, args->length, 10000,
								 args->callback);
				break;
			case NimbleCallbackReason::UNKNOWN:
				ESP_LOGI(tag, "Yobarenai hazu");
				break;
		}
		return 0;
	};

	args->central->connect(args->address, args->callback);

	return 0;
}

bool NXSWirelessClient::disconnect() {
	if (central->disconnect()) return false;
	return true;
}

bool NXSWirelessClient::send_async(const uint8_t *command, size_t length) {
	static callback_args_t *args = nullptr;

	if (args != nullptr) {
		ESP_LOGI(tag, "busy");
		return false;
	}

	args				 = new callback_args_t();
	args->address		 = &address;
	args->central		 = central;
	args->service		 = (const ble_uuid_t *)&service;
	args->characteristic = (const ble_uuid_t *)&control_characteristic;
	args->command		 = command;
	args->length		 = length;

	ESP_LOGI(tag, "start connect");

	static int try_count;
	static bool writed;

	try_count = 0;
	writed	= false;

	args->callback = [](uint16_t handle, NimbleCallbackReason reason) {
		switch (reason) {
			case NimbleCallbackReason::SUCCESS:
				ESP_LOGI(tag, "command write success");
				writed = true;
				break;
			case NimbleCallbackReason::CONNECTION_START:
			case NimbleCallbackReason::CHARACTERISTIC_WRITE_FAILED:
			case NimbleCallbackReason::CHARACTERISTIC_FIND_FAILED:
			case NimbleCallbackReason::SERVICE_FIND_FAILED:
			case NimbleCallbackReason::STOP_CANCEL_FAILED:
			case NimbleCallbackReason::CONNECTION_FAILED:
			case NimbleCallbackReason::OTHER_FAILED:
				if (++try_count > 3) return 1;
				ESP_LOGI(tag, "start connecting");
				args->central->connect(args->address, args->callback);
				break;
			case NimbleCallbackReason::CONNECTION_ESTABLISHED:
				ESP_LOGI(tag, "connected, start write");
				args->central->write(args->service, args->characteristic,
								 args->command, args->length, 10000,
								 args->callback);
				break;
			case NimbleCallbackReason::UNKNOWN:
				ESP_LOGI(tag, "Yobarenai hazu");
				break;
		}
		return 0;
	};

	args->callback(0, NimbleCallbackReason::CONNECTION_START);

	while (try_count < 3 && !writed) {
		ESP_LOGI(tag, "Try...");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	ESP_LOGI(tag, "Writing result: %d, by %d try", writed, try_count);
	delete args;
	args = nullptr;

	return writed;
}