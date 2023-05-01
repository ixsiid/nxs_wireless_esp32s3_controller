idf_component_register(SRCS "main.cpp" "nxs_wireless_client.cpp"
                    PRIV_REQUIRES nvs_flash simple-nimble-central
                    INCLUDE_DIRS ".")
