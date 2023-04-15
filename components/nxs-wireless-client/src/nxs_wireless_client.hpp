#pragma once

#include <nimble/ble.h>
#include <host/ble_uuid.h>
#include "nimble_central.hpp"

class NXSWirelessClient {
private:
	static bool is_nimble_started;
	static void nimble_start();

	ble_addr_t address;

	NimbleCentral * central;
	

	int send(const uint8_t * command, size_t length);
	bool send_async(const uint8_t *command, size_t length);
public:
	NXSWirelessClient(const char * peer_address);
	bool connect(const uint8_t * pin);
	bool disconnect();
	int up();
	int down();

	bool up_async();
	bool down_async();
	
	static const ble_uuid128_t service, control_characteristic, auth_characteristic;
	static  uint8_t command_shift_up[8];
	static  uint8_t command_shift_down[8];
};

inline int NXSWirelessClient::up() { return send(command_shift_up, sizeof(command_shift_up)); }
inline int NXSWirelessClient::down() { return send(command_shift_down, sizeof(command_shift_down)); }

inline bool NXSWirelessClient::up_async() { return send_async(command_shift_up, sizeof(command_shift_up)); }
inline bool NXSWirelessClient::down_async() { return send_async(command_shift_down, sizeof(command_shift_down)); }
