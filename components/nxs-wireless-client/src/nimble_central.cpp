#include <esp_log.h>
#include <nvs_flash.h>

#include <host/ble_gap.h>
#include <services/gap/ble_svc_gap.h>
#include <nimble/nimble_port_freertos.h>
#include <nimble/nimble_port.h>
#include <esp_nimble_hci.h>
#include <host/util/util.h>

#include "misc.h"

#include "nimble_central.hpp"

const char *tag = "NimBleCentral";

bool NimbleCentral::is_started = false;

int NimbleCentral::start(const char *device_name) {
	ESP_LOGI(tag, "start");
	if (is_started) return 0;

	int rc;

	ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());

	nimble_port_init();
	/* Configure the host. */
	ble_hs_cfg.reset_cb = [](int reason) {};
	ble_hs_cfg.sync_cb	= []() {
		 int rc;

		 /* Make sure we have proper identity address set (public preferred) */
		 rc = ble_hs_util_ensure_addr(0);
		 assert(rc == 0);
	};
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	/* Set the default device name. */
	rc = ble_svc_gap_device_name_set(device_name);

	nimble_port_freertos_init([](void *param) {
		ESP_LOGI(tag, "BLE Host Task Started");
		/* This function will return only when nimble_port_stop() is executed */
		nimble_port_run();	//

		ESP_LOGI(tag, "nimble_port_freertos_deinit");
		nimble_port_freertos_deinit();
	});

	is_started = true;
	ESP_LOGI(tag, "started %d", rc);
	return 0;
}

typedef struct {
	NimbleCallback callback;
	const ble_uuid_t *service;
	const ble_uuid_t *characteristic;
	bool found_service;
	uint16_t service_start_handle;
	uint16_t service_end_handle;
	bool found_characteristic;
	const uint8_t *value;
	size_t length;
} gattc_callback_args_t;

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that is
 * established.  blecent uses the same callback for all connections.
 *
 * @param event                 The event being signalled.
 * @param arg                   Application-specified argument; unused by
 *                                  blecent.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
int NimbleCentral::blecent_gap_event(struct ble_gap_event *event, void *arg) {
	struct ble_gap_conn_desc desc;
	int rc;

	ESP_LOGV(tag, "blecent_gap_event: %d", event->type);

	NimbleCallback callback = (NimbleCallback)arg;

	switch (event->type) {
		case BLE_GAP_EVENT_CONNECT:
			/* A new connection was established or a connection attempt failed. */
			if (event->connect.status == 0) {
				/* Connection successfully established. */
				ESP_LOGI(tag, "BLE_GAP_EVENT_CONNECT: success");
				rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
				if (callback) callback(event->connect.conn_handle, NimbleCallbackReason::CONNECTION_ESTABLISHED);
			} else {
				/* Connection attempt failed; resume scanning. */
				// MODLOG_DFLT(ERROR, "Error: Connection failed; status=%d\n", event->connect.status);
				ESP_LOGI(tag, "BLE_GAP_EVENT_CONNECT: failed");

				if (callback) callback(event->connect.conn_handle, NimbleCallbackReason::CONNECTION_FAILED);
			}

			return 0;

		case BLE_GAP_EVENT_REPEAT_PAIRING:
			/* We already have a bond with the peer, but it is attempting to
			 * establish a new secure link.  This app sacrifices security for
			 * convenience: just throw away the old bond and accept the new link.
			 */

			/* Delete the old bond. */
			rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
			assert(rc == 0);
			ble_store_util_delete_peer(&desc.peer_id_addr);

			/* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
			 * continue with the pairing operation.
			 */
			return BLE_GAP_REPEAT_PAIRING_RETRY;

		case BLE_GAP_EVENT_DISCONNECT:
			ESP_LOGI(tag, "BLE_GAP_EVENT_DISCONNECT: %d", event->connect.conn_handle);
			if (callback) callback(event->connect.conn_handle, NimbleCallbackReason::DISCONNECT);
			return 0;
		default:
			return 0;
	}
}

int NimbleCentral::connect(const ble_addr_t *address, NimbleCallback callback) {
	int rc;
	assert(callback != nullptr);

	rc = ble_gap_connect(0, address, 4000, nullptr, blecent_gap_event, (void *)callback);
	ESP_LOGI(tag, "gap connect result: %d", rc);
	if (rc == BLE_HS_EDONE) callback(0x0000, NimbleCallbackReason::CONNECTION_ESTABLISHED);
	else if (rc) callback(0x0000, NimbleCallbackReason::CONNECTION_FAILED);

	return rc;
}

int NimbleCentral::disconnect() {
	ESP_LOGI(tag, "disconnect");
	// handleが常に0なのは謎
	ble_gap_terminate(0x0000, BLE_ERR_REM_USER_CONN_TERM);
	return 0;
}

int NimbleCentral::chr_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
						const struct ble_gatt_chr *chr, void *arg) {
	int rc;

	ESP_LOGI(tag, "chara event status: %d", error->status);

	gattc_callback_args_t *client = (gattc_callback_args_t *)arg;

	switch (error->status) {
		case 0:
			ESP_LOGI(tag, "start write characteristic");
			client->found_characteristic = true;

			rc = ble_gattc_write_no_rsp_flat(conn_handle, chr->val_handle, client->value, client->length);
			if (client->callback != nullptr) client->callback(conn_handle, rc == 0 ? NimbleCallbackReason::SUCCESS : NimbleCallbackReason::CHARACTERISTIC_WRITE_FAILED);
			if (rc == 0) ESP_LOGI(tag, "success");

			break;

		case BLE_HS_EDONE:
			ESP_LOGI(tag, "chr EDONE");
			if (client->callback != nullptr) client->callback(conn_handle, NimbleCallbackReason::DONE);
			rc = error->status;
			break;
		case BLE_HS_EALREADY:
		case BLE_HS_EBUSY:
		default:
			rc = error->status;
			break;
	}

	if (rc != 0 && !client->found_characteristic) {
		ESP_LOGE(tag, "Failed find chr or write characteristic");

		if (client->callback != nullptr) client->callback(conn_handle, NimbleCallbackReason::CHARACTERISTIC_FIND_FAILED);

		// delete arg;
		delete client;
	}

	ESP_LOGI(tag, "Finish cmd press");

	return rc;
}

int NimbleCentral::svc_disced(uint16_t conn_handle, const struct ble_gatt_error *error,
						const struct ble_gatt_svc *service, void *arg) {
	int rc = 0;

	ESP_LOGI(tag, "service event status: %d", error->status);

	gattc_callback_args_t *client = (gattc_callback_args_t *)arg;

	switch (error->status) {
		case 0:  // Success
			client->found_service = true;

			client->service_start_handle = service->start_handle;
			client->service_end_handle   = service->end_handle;
			break;

		// ENOTCONNが呼ばれたときに、EDONEが呼ばれないかは不明
		case BLE_HS_ENOTCONN:
			ESP_LOGI(tag, "not connection: %d", conn_handle);
			if (client->callback) client->callback(conn_handle, NimbleCallbackReason::CONNECTION_FAILED);
			delete client;
			break;
		case BLE_HS_EDONE:
			ESP_LOGI(tag, "done");
			if (client->found_service) {
				ESP_LOGI(tag, "Finding service finish");
				rc = ble_gattc_disc_chrs_by_uuid(conn_handle,
										   client->service_start_handle,
										   client->service_end_handle,
										   client->characteristic, chr_disced, client);
			} else {
				ESP_LOGE(tag, "Couldn't find service uuid.");
				ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
				if (client->callback) client->callback(conn_handle, NimbleCallbackReason::SERVICE_FIND_FAILED);
				rc = error->status;

				// delete arg;
				delete client;
			}
			break;

		default:
			rc = error->status;
			break;
	}

	if (rc != 0) {
		ESP_LOGE(tag, "Service discover error: %d", rc);
	}

	return rc;
}

int NimbleCentral::write(const ble_uuid_t *service, const ble_uuid_t *characteristic,
					const uint8_t *value, size_t length, int timeout,
					NimbleCallback callback) {
	int rc;
	gattc_callback_args_t *arg = new gattc_callback_args_t();
	arg->callback			  = callback;
	arg->service			  = service;
	arg->characteristic		  = characteristic;
	arg->found_service		  = false;
	arg->found_characteristic  = false;
	arg->value			  = value;
	arg->length			  = length;

	// handleが常に0なのは謎
	rc = ble_gattc_disc_svc_by_uuid(0x0000, service, svc_disced, arg);

	if (rc != 0) {
		ESP_LOGI(tag, "Failed find characteristics");
		if (callback) callback(0x0000, NimbleCallbackReason::CONNECTION_FAILED);
	}

	return rc;
}
