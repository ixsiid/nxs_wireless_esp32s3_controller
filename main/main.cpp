/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "esp_log.h"
#include "nvs_flash.h"

#include <nxs_wireless_client.hpp>
#include <button.hpp>

#include "nxs_macaddr.h"
// #define NXS_MAC "your_nxs_quantumm_mac_address" // "01:23:45:68:89:ab"
// #define NXS_PIN "your_nxs_quantumm_pin" // {0x01, 0x01, 0x01, 0x01}

#define tag "NXS"

extern "C" {
void app_main();
}

#include "RMT_WS2812.hpp"

#define CONFIG_MAX_BRIGHTNESS 20

void app_main(void) {
	ESP_LOGI(tag, "Start");
	esp_err_t ret;

	/* Initialize NVS — it is used to store PHY calibration data */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// static NXSWirelessClient *nxs = new NXSWirelessClient(NXS_MAC);
	// nxs->connect(NXS_PIN);

	// LED
	static RMT_WS2812 *led = new RMT_WS2812(RMT_WS2812::esp_board::ATOMS3_lite);
	led->clear();

	led->setBrightness(CONFIG_MAX_BRIGHTNESS);

	// 起動時に青で点滅
	led->setPixel(0, 0, 0, 255);
	led->refresh();
	vTaskDelay(200 / portTICK_PERIOD_MS);
	led->clear();

	// ATOMS3
	/// G6をGND代わりに使う
	const gpio_num_t gnd_pin = gpio_num_t::GPIO_NUM_5;
	gpio_set_direction(gnd_pin, gpio_mode_t::GPIO_MODE_OUTPUT);
	gpio_set_level(gnd_pin, 0);

	/// High, Lowボタンのピン番号
	const uint8_t high_pin = 6;
	const uint8_t low_pin  = 7;

	const uint8_t buttonPins[] = {high_pin, low_pin};
	static Button *button	  = new Button(buttonPins, sizeof(buttonPins));

	static bool lighting;
	while (true) {
		// Main loop
		vTaskDelay(50 / portTICK_RATE_MS);

		button->check(nullptr, [](uint8_t pin) {
			switch (pin) {
				case high_pin:
					ESP_LOGI(tag, "button: high");
					// nxs->up();
					break;
				case low_pin:
					ESP_LOGI(tag, "button: low");
					// nxs->down();
					break;
			}
		});
	}
}
