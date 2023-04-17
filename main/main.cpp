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

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>

#include <nxs_wireless_client.hpp>

#include "nxs_macaddr.h"
// #define NXS_MAC "your_nxs_quantumm_mac_address" // "01:23:45:68:89:ab"
// #define NXS_PIN "your_nxs_quantumm_pin" // {0x01, 0x01, 0x01, 0x01}

#define tag "NXS"

extern "C" {
void app_main();
}

#include "RMT_WS2812.hpp"

#define CONFIG_MAX_BRIGHTNESS 20

void sleep() {
	ESP_LOGI(tag, "Goto sleep");

	gpio_num_t ext_wakeup_pin_1 = gpio_num_t::GPIO_NUM_6;
	gpio_num_t ext_wakeup_pin_2 = gpio_num_t::GPIO_NUM_7;

	esp_sleep_enable_ext1_wakeup(
	    BIT64(ext_wakeup_pin_1) | BIT64(ext_wakeup_pin_2),	 // | BIT64(GPIO_NUM_41),
	    ESP_EXT1_WAKEUP_ANY_HIGH);

	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
	rtc_gpio_pullup_dis(ext_wakeup_pin_1);
	rtc_gpio_pulldown_en(ext_wakeup_pin_1);
	rtc_gpio_pullup_dis(ext_wakeup_pin_2);
	rtc_gpio_pulldown_en(ext_wakeup_pin_2);

	gpio_num_t pull_up_pin = gpio_num_t::GPIO_NUM_5;
	rtc_gpio_init(pull_up_pin);
	rtc_gpio_set_direction(pull_up_pin, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
	rtc_gpio_set_direction_in_sleep(pull_up_pin, rtc_gpio_mode_t::RTC_GPIO_MODE_OUTPUT_ONLY);
	rtc_gpio_set_level(pull_up_pin, 1);

	esp_deep_sleep_start();
}

enum class Action {
	None,
	Up,
	Down,
};

void app_main(void) {
	ESP_LOGI(tag, "Start");
	esp_err_t ret;

	esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
	if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT1) sleep();

	Action act		= Action::None;
	uint64_t wakeup_pin = esp_sleep_get_ext1_wakeup_status();
	ESP_LOGI(tag, "wake up pin: %lld", wakeup_pin);
	if (wakeup_pin & (1ULL << 6)) {
		ESP_LOGI(tag, "pin 6");
		act = Action::Up;
	}
	if (wakeup_pin & (1ULL << 7)) {
		ESP_LOGI(tag, "pin 7");
		act = Action::Down;
	}

	if (act == Action::None) sleep();

	/* Initialize NVS — it is used to store PHY calibration data */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	static NXSWirelessClient *nxs = new NXSWirelessClient(NXS_MAC);

	int retry = 3;
	switch (act) {
		case Action::Up:
			while (retry--) {
				if (nxs->connect_up_disconnect(NXS_PIN)) break;
			}
			break;
		case Action::Down:
			while (retry--) {
				if (nxs->connect_down_disconnect(NXS_PIN)) break;
			}
			break;
		default:
			break;
	}
	sleep();
}
