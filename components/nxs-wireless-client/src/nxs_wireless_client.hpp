#pragma once

#include <nimble/ble.h>
#include <host/ble_uuid.h>
#include "nimble_central.hpp"

#include <freertos/event_groups.h>

class NXSWirelessClient {
    private:
	static const EventBits_t EVENT_FAILED	  = 1 << 0;
	static const EventBits_t EVENT_DONE	  = 1 << 1;
	static const EventBits_t EVENT_CONNECTED  = 1 << 3;
	static const EventBits_t EVENT_CHR_WRITED = 1 << 6;

    private:
	static bool is_nimble_started;
	static void nimble_start();

	static EventGroupHandle_t event_group;

	ble_addr_t address;

	NimbleCentral *central;

	int send(const uint8_t *command, size_t length);
	bool connect_send_disconnect(const uint8_t *pin, const uint8_t *command, size_t length);

    public:
	NXSWirelessClient(const char *peer_address);
	bool connect(const uint8_t *pin);
	bool disconnect();
	int up();
	int down();

	bool connect_up_disconnect(const uint8_t *pin);
	bool connect_down_disconnect(const uint8_t *pin);

	static const ble_uuid128_t service, control_characteristic, auth_characteristic;
	static uint8_t command_shift_up[8];
	static uint8_t command_shift_down[8];
};

inline int NXSWirelessClient::up() { return send(command_shift_up, sizeof(command_shift_up)); }
inline int NXSWirelessClient::down() { return send(command_shift_down, sizeof(command_shift_down)); }

inline bool NXSWirelessClient::connect_up_disconnect(const uint8_t *pin) { return connect_send_disconnect(pin, command_shift_up, sizeof(command_shift_up)); }
inline bool NXSWirelessClient::connect_down_disconnect(const uint8_t *pin) { return connect_send_disconnect(pin, command_shift_down, sizeof(command_shift_down)); }
