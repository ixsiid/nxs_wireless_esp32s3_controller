#pragma once

#include <nimble/ble.h>
#include <host/ble_uuid.h>
#include "simple_nimble_central.hpp"

class NXSWirelessClient {
    private:
	ble_addr_t address;

	SimpleNimbleCentral *central;

	bool send(const uint8_t *command, size_t length);
	bool connect_send_disconnect(const uint8_t *pin, const uint8_t *command, size_t length);

    public:
	NXSWirelessClient(const char *peer_address);
	bool connect(const uint8_t *pin);
	bool disconnect();
	bool up();
	bool down();

	bool connect_up_disconnect(const uint8_t *pin);
	bool connect_down_disconnect(const uint8_t *pin);

	static const ble_uuid128_t service, control_characteristic, auth_characteristic;
	static uint8_t command_shift_up[8];
	static uint8_t command_shift_down[8];
};

inline bool NXSWirelessClient::up() { return send(command_shift_up, sizeof(command_shift_up)); }
inline bool NXSWirelessClient::down() { return send(command_shift_down, sizeof(command_shift_down)); }

inline bool NXSWirelessClient::connect_up_disconnect(const uint8_t *pin) { return connect_send_disconnect(pin, command_shift_up, sizeof(command_shift_up)); }
inline bool NXSWirelessClient::connect_down_disconnect(const uint8_t *pin) { return connect_send_disconnect(pin, command_shift_down, sizeof(command_shift_down)); }
