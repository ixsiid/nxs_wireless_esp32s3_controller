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

#define CONFIG_RMT_CHANNEL     RMT_CHANNEL_0
#define CONFIG_MAX_BRIGHTNESS  20
#define CONFIG_RMT_GPIO        GPIO_NUM_35
#define CONFIG_LED_NUM         1

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

	// static NXSWirelessClient *sb = new NXSWirelessClient(NXS_MAC);
	// sb->connect(NXS_PIN);

	// LED
	RMT_WS2812 *led = new RMT_WS2812(CONFIG_RMT_CHANNEL, CONFIG_RMT_GPIO, CONFIG_LED_NUM);
	ESP_LOGI(tag, "%p", led);
	led->begin();
	led->clear();
	/*

	// ATOMS3
	const uint8_t buttonPins[] = {41, 1, 2};
	static Button *button = new Button(buttonPins, sizeof(buttonPins));

	xTaskCreatePinnedToCore([](void *_) {
		while (true) {
			// Main loop
			vTaskDelay(100 / portTICK_RATE_MS);

			button->check(nullptr, [&](uint8_t pin) {
				switch (pin) {
					case 1:
						ESP_LOGI(tag, "released gpio 1");
						// sb->up();
						break;
					case 2:
						ESP_LOGI(tag, "released gpio 2");
						// sb->down();
						break;
					case 41:
						ESP_LOGI(tag, "released gpio 41");
						break;
				}
			});
		}
	}, "ButtonCheck", 4096, nullptr, 1, nullptr, 1);

	*/
	int color = 0;
	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		ESP_LOGD(tag, "Idleing");
		
		led->setPixel(0, 
			color & 0b001 ? 255 : 0,
			color & 0b010 ? 255 : 0,
			color & 0b100 ? 255 : 0);
		led->setBrightness(CONFIG_MAX_BRIGHTNESS);
		led->refresh();
		++color;
		ESP_LOGI(tag, "color: %d", color);
	}
}
