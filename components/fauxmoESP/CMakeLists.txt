idf_component_register(SRCS "fauxmoESP.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_server esp_netif esp_wifi nvs_flash driver esp_event)