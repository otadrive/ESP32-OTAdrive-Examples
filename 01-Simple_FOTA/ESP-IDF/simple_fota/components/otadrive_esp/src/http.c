#include "otd_common.h"
#include "otadrive_esp.h"
#include "otd_privates.h"
#include "esp_mac.h"
#include <esp_rom_spiflash.h>

esp_err_t otd_http_event_handler(esp_http_client_event_t *evt)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "http hdr %s %s", evt->header_key, evt->header_value);
        if (strncasecmp(evt->header_key, "X-Version", 20) == 0)
        {
            strncpy(otadrv_hdl.result.hdr_ver, evt->header_value, CONFIG_OTADRIVE_VER_LEN);
        }
        else if (strncasecmp(evt->header_key, "Content-Length", 20) == 0)
        {
            otadrv_hdl.result.hdr_length = atoi(evt->header_value);
            allocate_buf(&otadrv_hdl.result.body_buffer, otadrv_hdl.result.hdr_length + 1);
        }

        break;
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            char *buf = evt->data;
            if (evt->data_len <= otadrv_hdl.result.hdr_length)
            {
                memcpy(otadrv_hdl.result.body_buffer, buf, evt->data_len);
                ESP_LOGI(TAG, "http data: %s %dBytes", otadrv_hdl.result.body_buffer, evt->data_len);
            }
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
    {
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGE(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGE(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    }
#pragma GCC diagnostic pop
    return ESP_OK;
}

void set_header_int(esp_http_client_handle_t client, char *key, uint32_t value)
{
    char str[33];
    itoa(value, str, 10);
    esp_http_client_set_header(client, key, str);
}

void fill_request_headers(esp_http_client_handle_t client)
{
    esp_http_client_set_header(client, "User-Agent", "OTAdrive-IDFSDK-v2.1.1");
    esp_http_client_set_header(client, "Cache-Control", "no-cache");
    if (otadrv_hdl.useMd5)
    {
        esp_http_client_set_header(client, "x-check-sketch-md5", "1");
        esp_http_client_set_header(client, "x-ESP32-sketch-md5", getSketchMD5String());
    }
    set_header_int(client, "x-ESP32-chip-size", g_rom_flashchip.chip_size);
    set_header_int(client, "x-ESP32-free-space", getFreeSketchSpace());
    set_header_int(client, "x-ESP32-sketch-size", sketchSize(false));

    set_header_int(client, "x-ESP32-free-heap", esp_get_free_heap_size());
    set_header_int(client, "x-ESP32-min-heap", esp_get_minimum_free_heap_size());
    esp_http_client_set_header(client, "x-ESP32-sdk-version", esp_get_idf_version());
}
