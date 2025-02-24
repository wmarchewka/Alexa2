#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "fauxmoESP.h"
#include "driver/gpio.h"
#include "esp_event.h"

static const char *TAG = "MAIN";
FauxmoESP *fauxmo;

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Wi-Fi started, connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGE(TAG, "Wi-Fi disconnected, retrying...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifiSetup()
{
    ESP_LOGI(TAG, "Initializing NVS...");
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "Setting up Wi-Fi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_event_group = xEventGroupCreate();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "WALTERMARCHEWKA",
            .password = "Alignment67581",
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .bssid_set = false,
            .bssid = {0},
            .channel = 0, // 0 means auto-select channel
            .listen_interval = 0,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {
                .rssi = -127, // Default to lowest possible RSSI threshold
                .authmode = WIFI_AUTH_WPA2_PSK,
                .rssi_5g_adjustment = 0},
            .pmf_cfg = {.capable = true, .required = false},
            .rm_enabled = false,
            .btm_enabled = false,
            .mbo_enabled = false,
            .ft_enabled = false,
            .owe_enabled = false,
            .transition_disable = false,
            .reserved = 0,
            .sae_pwe_h2e = WPA3_SAE_PWE_HUNT_AND_PECK, // Default for WPA3 SAE-PWE method
            .sae_pk_mode = WPA3_SAE_PK_MODE_DISABLED,  // Default for WPA3 SAE-PK mode
            .failure_retry_cnt = 0,
            .he_dcm_set = false,
            .he_dcm_max_constellation_tx = 0,
            .he_dcm_max_constellation_rx = 0,
            .he_mcs9_enabled = false,
            .he_su_beamformee_disabled = false,
            .he_trig_su_bmforming_feedback_disabled = false,
            .he_trig_mu_bmforming_partial_feedback_disabled = false,
            .he_trig_cqi_feedback_disabled = false,
            .he_reserved = 0,
            .sae_h2e_identifier = {0}}};

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for IP...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "Starting FauxmoESP...");
    wifiSetup();
    fauxmo = new FauxmoESP();
    fauxmo->enable(true);
    fauxmo->addDevice("Lamp");

    while (true)
    {
        fauxmo->handle();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}