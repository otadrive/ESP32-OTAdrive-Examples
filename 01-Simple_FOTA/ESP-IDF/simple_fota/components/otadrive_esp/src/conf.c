#include "otadrive_esp.h"
#include "otd_privates.h"
#include "otd_common.h"

static char *config_buffer = NULL;
static size_t config_buffer_size = 0;

bool downloadConfigValues()
{
    if (xSemaphoreTake(otadrv_lock, pdMS_TO_TICKS(1000)) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to get lock");
        return false;
    }

    char url[CONFIG_OTADRIVE_URL_LEN];
    snprintf(url, CONFIG_OTADRIVE_URL_LEN, "%s/config?plain&k=%s&v=%s&s=%s", otadrv_hdl.serverurl, otadrv_hdl.apiKey, otadrv_hdl.current_version, otadrv_hdl.serial);
    ESP_LOGI(TAG, "Getting config from %s", url);
    esp_http_client_config_t httpconfig = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .cert_pem = OTADRIVE_CERT,
        .event_handler = otd_http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&httpconfig);
    esp_err_t err = esp_http_client_perform(client);
    bool success = false;
    if (err == ESP_OK)
    {
        esp_http_client_fetch_headers(client);

        switch (esp_http_client_get_status_code(client))
        {
        case 200:
            success = true;
            allocate_buf(&config_buffer, otadrv_hdl.result.hdr_length + 1);
            memcpy(config_buffer, otadrv_hdl.result.body_buffer, otadrv_hdl.result.hdr_length);
            config_buffer_size = otadrv_hdl.result.hdr_length;

            ESP_LOGI(TAG, "Config = %sCEND", config_buffer);
            /* FALLTHROUGH */
        case 304:
        case 401:
        case 404:
            break;
        default:
            break;
        }
        ESP_LOGI(TAG, "HTTP GET Status = %d", //, content_length = %" PRId64 "",
                 esp_http_client_get_status_code(client));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET Error = %d", err);
    }

    esp_http_client_cleanup(client);
    xSemaphoreGive(otadrv_lock);
    return success;
}

bool getConfigValue(char *key, char *o_value, int o_maxlen)
{
    if (config_buffer_size == 0)
        return false;
    if (config_buffer == NULL)
        return false;

    ESP_LOGI(TAG, "Search in %s for %s", config_buffer, key);

    for (uint32_t i = 0; i < config_buffer_size; i++)
    {
        uint32_t j = 0;
        for (; key[j] != '\0'; j++)
        {
            if (config_buffer[i + j] != key[j])
                break;
        }
        if (key[j] != '\0')
            continue;

        // key founded
        if (config_buffer[i + j] != '=')
            continue;
        j = i + j + 1;

        // copy value
        o_value[o_maxlen - 1] = '\0';
        for (uint32_t io = 0; io < o_maxlen - 1; io++, j++)
        {
            if (config_buffer[j] == '\\' && config_buffer[j + 1] == 'n')
            {
                o_value[io] = '\n';
                j++;
                continue;
            }
            else if (config_buffer[j] == '\\' && config_buffer[j + 1] == 'r')
            {
                o_value[io] = '\r';
                j++;
                continue;
            }
            else if (config_buffer[j] == '\n')
            {
                o_value[io] = '\0';
                return true;
            }
            o_value[io] = config_buffer[j];
        }
    }

    return false;
}