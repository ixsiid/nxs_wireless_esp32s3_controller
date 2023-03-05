idf_component_register(SRCS "main.cpp" "button.cpp"
                    PRIV_REQUIRES nvs_flash nxs-wireless-client simple_ws2812_rmt_esp-idf
                    INCLUDE_DIRS ".")
