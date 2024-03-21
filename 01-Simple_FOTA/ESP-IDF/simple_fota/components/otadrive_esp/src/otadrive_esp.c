#include <stdlib.h>
#include <fnmatch.h>
#include <libgen.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_http_client.h>
#include <esp_tls.h>
#include <esp_crt_bundle.h>
#include <esp_log.h>
#include <esp_app_format.h>
#include <esp_ota_ops.h>
#include <esp_https_ota.h>
#include <esp_event.h>

#include <sdkconfig.h>
#include "otadrive_esp.h"
#include "esp_mac.h"
#include <esp_timer.h>

#define OTADRIVE_CERT "-----BEGIN CERTIFICATE-----\n\
MIIDuzCCAqOgAwIBAgIDBETAMA0GCSqGSIb3DQEBBQUAMH4xCzAJBgNVBAYTAlBM\n\
MSIwIAYDVQQKExlVbml6ZXRvIFRlY2hub2xvZ2llcyBTLkEuMScwJQYDVQQLEx5D\n\
ZXJ0dW0gQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkxIjAgBgNVBAMTGUNlcnR1bSBU\n\
cnVzdGVkIE5ldHdvcmsgQ0EwHhcNMDgxMDIyMTIwNzM3WhcNMjkxMjMxMTIwNzM3\n\
WjB+MQswCQYDVQQGEwJQTDEiMCAGA1UEChMZVW5pemV0byBUZWNobm9sb2dpZXMg\n\
Uy5BLjEnMCUGA1UECxMeQ2VydHVtIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MSIw\n\
IAYDVQQDExlDZXJ0dW0gVHJ1c3RlZCBOZXR3b3JrIENBMIIBIjANBgkqhkiG9w0B\n\
AQEFAAOCAQ8AMIIBCgKCAQEA4/t9o3K6wvDJFIf1awFO4W5AB7ptJ11/91sts1rH\n\
UV+rpDKmYYe2bg+G0jACl/jXaVehGDldamR5xgFZrDwxSjh80gTSSyjoIF87B6LM\n\
TXPb865Px1bVWqeWifrzq2jUI4ZZJ88JJ7ysbnKDHDBy3+Ci6dLhdHUZvSqeexVU\n\
BBvXQzmtVSjF4hq79MDkrjhJM8x2hZ85RdKknvISjFH4fOQtf/WsX+sWn7Et0brM\n\
kUJ3TCXJkDhv2/DM+44el1k+1WBO5gUo7Ul5E0u6SNsv+XLTOcr+H9g0cvW0QM8x\n\
AcPs3hEtF10fuFDRXhmnad4HMyjKUJX5p1TLVIZQRan5SQIDAQABo0IwQDAPBgNV\n\
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBQIds3LB/8k9sXN7buQvOKEN0Z19zAOBgNV\n\
HQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQEFBQADggEBAKaorSLOAT2mo/9i0Eidi15y\n\
sHhE49wcrwn9I0j6vSrEuVUEtRCjjSfeC4Jj0O7eDDd5QVsisrCaQVymcODU0HfL\n\
I9MA4GxWL+FpDQ3Zqr8hgVDZBqWo/5U30Kr+4rP1mS1FhIrlQgnXdAIv94nYmem8\n\
J9RHjboNRhx3zxSkHLmkMcScKHQDNP8zGSal6Q10tz6XxnboJ5ajZt3hrvJBW8qY\n\
VoNzcOSGGtIxQbovvi0TWnZvTuhOgQ4/WwMioBK+ZlgRSssDxLQqKi2WF+A5VLxI\n\
03YnnZotBqbJ7DnSq9ufmgsnAjUpsUCV5/nonFWIGUbWtzT1fs45mtk48VH3Tyw=\n\
-----END CERTIFICATE-----"

#define CONFIG_OTADRIVE_URL_LEN 160

#ifndef CONFIG_OTADRIVE_URL
#define CONFIG_OTADRIVE_URL "otadrive.com/deviceapi"
#endif

#ifndef CONFIG_OTADRIVE_HTTPS
#define CONFIG_OTADRIVE_HTTPS 1
#endif

#ifndef CONFIG_OTADRIVE_MD5
#define CONFIG_OTADRIVE_MD5 1
#endif

static const char *TAG = "OTADRIVE";

ESP_EVENT_DEFINE_BASE(OTADRIVE_EVENTS);

bool allocate_buf(char **buf, uint32_t size);
bool free_buf(char **buf);

typedef struct otadrive_session_t
{
    char apiKey[40];          /*!< The APIkey of the product */
    char current_version[24]; /*!< Version code of the current firmware */
    char serial[20];          /*!< Chip serial number */
    char serverurl[40];       /*!< Base URL of the APIs */
    char app_md5[40];
    bool useMd5;
    bool useHTTPS;
    struct
    {
        char hdr_ver[CONFIG_OTADRIVE_VER_LEN];
        uint32_t hdr_length;
        char *body_buffer;
    } result;

    TaskHandle_t task_handle;
    const esp_partition_t *storage_partition;
} otadrive_session_t;
typedef struct otadrive_session_t otadrive_session_t;
static otadrive_session_t otadrv_hdl;

SemaphoreHandle_t otadrv_lock = NULL;

/*String otadrive_ota::getChipId()
{
    String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
    ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
    return ChipIdHex;
}*/

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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

void otadrive_setInfo(char *apiKey, char *current_version)
{
    if (!otadrv_lock)
        otadrv_lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(otadrv_lock, pdMS_TO_TICKS(1000)) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to take lock");
        return;
    }

    ESP_LOGI(TAG, "Initializing");
    bzero((void *)&otadrv_hdl, sizeof(otadrive_session_t));
    otadrv_hdl.useHTTPS = CONFIG_OTADRIVE_URL;
    otadrv_hdl.useMd5 = CONFIG_OTADRIVE_MD5;
    snprintf(otadrv_hdl.serverurl, CONFIG_OTADRIVE_URL_LEN, "http%s://%s", otadrv_hdl.useHTTPS ? "s" : "", CONFIG_OTADRIVE_URL);
    strcpy(otadrv_hdl.apiKey, apiKey);
    strcpy(otadrv_hdl.current_version, current_version);
    uint64_t _chipmacid = 0LL;
    esp_efuse_mac_get_default((uint8_t *)(&_chipmacid));

    sprintf(otadrv_hdl.serial, "%llx", _chipmacid);

    ESP_LOGI(TAG, "Chip serial:%s, App version: %s", otadrv_hdl.serial, otadrv_hdl.current_version);

    // esp_app_get_description

    xSemaphoreGive(otadrv_lock);
}

bool getJsonConfigs(char *json)
{
    return false;
}

static char *config_buffer = NULL;
static size_t config_buffer_size = 0;
otadrive_config_item *o_items = NULL;
bool getConfigValues()
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
        .event_handler = _http_event_handler,
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
            ESP_LOGI(TAG, "index %lu %c", j, config_buffer[j]);
            if (config_buffer[j] == '\n')
            {
                o_value[io] = '\0';
                return true;
            }
            o_value[io] = config_buffer[j];
        }
    }

    return false;
}

otadrive_result otadrive_updateFirmwareInfo()
{
    otadrive_result r;
    bzero(&r, sizeof(otadrive_result));

    if (xSemaphoreTake(otadrv_lock, pdMS_TO_TICKS(1000)) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to get lock");
        return r;
    }
    ESP_LOGI(TAG, "Checking for new release");
    ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, OTADRIVE_EVENT_START_CHECK, &otadrv_hdl, sizeof(otadrive_session_t *), portMAX_DELAY));

    char url[CONFIG_OTADRIVE_URL_LEN];
    snprintf(url, CONFIG_OTADRIVE_URL_LEN, "%s/update?k=%s&v=%s&s=%s", otadrv_hdl.serverurl, otadrv_hdl.apiKey, otadrv_hdl.current_version, otadrv_hdl.serial);
    ESP_LOGI(TAG, "Searching for Firmware from %s", url);
    esp_http_client_config_t httpconfig = {
        .url = url,
        .method = HTTP_METHOD_HEAD,
        .cert_pem = OTADRIVE_CERT,
        .event_handler = _http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&httpconfig);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        esp_http_client_fetch_headers(client);
        int status_code;
        status_code = esp_http_client_get_status_code(client);

        switch (status_code)
        {
        case 200:
            r.available_size = esp_http_client_get_content_length(client);
            strncpy(r.available_version, otadrv_hdl.result.hdr_ver, CONFIG_OTADRIVE_VER_LEN - 1);
            /* FALLTHROUGH */
        case 304:
        case 401:
        case 404:
            break;
        default:
            r.code = OTADRIVE_NoFirmwareExists;
            break;
        }
        ESP_LOGI(TAG, "HTTP GET Status = %d, %s", //, content_length = %" PRId64 "",
                 status_code, esp_err_to_name(status_code));
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET Error = %d", err);
    }

    esp_http_client_cleanup(client);
    xSemaphoreGive(otadrv_lock);

    return r;
}

static esp_err_t validate_image_header(esp_app_desc_t *new_app_info)
{
    if (new_app_info == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_LOGI(TAG, "New Firmware Details:");
    ESP_LOGI(TAG, "Project name: %s", new_app_info->project_name);
    ESP_LOGI(TAG, "Firmware version: %s", new_app_info->version);
    ESP_LOGI(TAG, "Compiled time: %s %s", new_app_info->date, new_app_info->time);
    ESP_LOGI(TAG, "ESP-IDF: %s", new_app_info->idf_ver);
    ESP_LOGI(TAG, "SHA256:");
    ESP_LOG_BUFFER_HEX(TAG, new_app_info->app_elf_sha256, sizeof(new_app_info->app_elf_sha256));

    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGD(TAG, "Current partition %s type %d subtype %d (offset 0x%08lx)",
             running->label, running->type, running->subtype, running->address);
    const esp_partition_t *update = esp_ota_get_next_update_partition(NULL);
    ESP_LOGD(TAG, "Update partition %s type %d subtype %d (offset 0x%08lx)",
             update->label, update->type, update->subtype, update->address);

#ifdef CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK
    /**
     * Secure version check from firmware image header prevents subsequent download and flash write of
     * entire firmware image. However this is optional because it is also taken care in API
     * esp_https_ota_finish at the end of OTA update procedure.
     */
    const uint32_t hw_sec_version = esp_efuse_read_secure_version();
    if (new_app_info->secure_version < hw_sec_version)
    {
        ESP_LOGW(TAG, "New firmware security version is less than eFuse programmed, %d < %d", new_app_info->secure_version, hw_sec_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}

static esp_err_t http_client_set_header_cb(esp_http_client_handle_t http_client)
{
    return esp_http_client_set_header(http_client, "Accept", "application/octet-stream");
}

esp_err_t _http_event_storage_handler(esp_http_client_event_t *evt)
{
    static int output_pos;
    static int last_progress;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_CONNECTED:
    {
        output_pos = 0;
        last_progress = 0;
        /* Erase the Partition */
        break;
    }
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            otadrive_session_t *handle = (otadrive_session_t *)evt->user_data;
            if (output_pos == 0)
            {
                ESP_LOGD(TAG, "Erasing Partition");
                ESP_ERROR_CHECK(esp_partition_erase_range(handle->storage_partition, 0, handle->storage_partition->size));
                ESP_LOGD(TAG, "Erasing Complete");
            }

            ESP_ERROR_CHECK(esp_partition_write(handle->storage_partition, output_pos, evt->data, evt->data_len));
            output_pos += evt->data_len;
            int progress = 100 * ((float)output_pos / (float)handle->storage_partition->size);
            if ((progress % 5 == 0) && (progress != last_progress))
            {
                ESP_LOGV(TAG, "Storage Firmware Update Progress: %d%%", progress);
                ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, OTADRIVE_EVENT_STORAGE_UPDATE_PROGRESS, &progress, sizeof(progress), portMAX_DELAY));
                last_progress = progress;
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

otadrive_result otadrive_updateFirmware()
{
    otadrive_result r;
    int64_t t_break = esp_timer_get_time() + (1000 * 1000 * 30);
    bzero(&r, sizeof(r));

    ESP_LOGI(TAG, "eeee %s ", esp_err_to_name(11));

    if (xSemaphoreTake(otadrv_lock, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGD(TAG, "Failed to take lock");
        return r;
    }

    char url[CONFIG_OTADRIVE_URL_LEN];
    snprintf(url, CONFIG_OTADRIVE_URL_LEN, "%s/update?k=%s&v=%s&s=%s", otadrv_hdl.serverurl, otadrv_hdl.apiKey, otadrv_hdl.current_version, otadrv_hdl.serial);
    ESP_LOGI(TAG, "Getting firmware from %s", url);

    esp_http_client_config_t httpconfig = {
        .url = url,
        .cert_pem = OTADRIVE_CERT,
        .keep_alive_enable = true,
        .buffer_size_tx = 4096,
        .timeout_ms = 30000,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &httpconfig,
        .http_client_init_cb = http_client_set_header_cb,
        .partial_http_download = true,
        .max_http_request_size = 4096};

    esp_https_ota_handle_t https_ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed: %d", err);
        goto ota_end;
    }

    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed: %d", err);
        goto ota_end;
    }
    err = validate_image_header(&app_desc);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "image header verification failed: %d", err);
        goto ota_end;
    }

    int32_t last_dl = -1;
    int last_progress = -1;
    int16_t fail_count = 0;
    while (1)
    {
        if (esp_timer_get_time() > t_break)
        {
            ESP_LOGE(TAG, "Total operation timeout");
            goto ota_end;
        }

        err = esp_https_ota_perform(https_ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            ESP_LOGI(TAG, "Error code: %d", err);
            break;
        }

        int32_t dl = esp_https_ota_get_image_len_read(https_ota_handle);
        int32_t size = esp_https_ota_get_image_size(https_ota_handle);

        // is anything received?
        if (last_dl != -1 && dl == last_dl)
        {
            fail_count++;
            if (fail_count == 5)
            {
                ESP_LOGE(TAG, "Too many receive fails");
                goto ota_end;
            }
        }
        else
        {
            fail_count = 0;
            t_break = esp_timer_get_time() + (1000 * 1000 * 120);
        }

        last_dl = dl;
        // calculate progress
        int progress = 100 * ((float)dl / (float)size);
        if ((progress % 5 == 0) && (progress != last_progress))
        {
            ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, OTADRIVE_EVENT_FIRMWARE_UPDATE_PROGRESS, &progress, sizeof(progress), portMAX_DELAY));
            ESP_LOGV(TAG, "Firmware Update Progress: %d%%", progress);
            last_progress = progress;
        }
    }

    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
    {
        // the OTA image was not completely received and user can customise the response to this situation.
        ESP_LOGE(TAG, "Complete data was not received.");
    }
    else
    {
        esp_err_t ota_finish_err = esp_https_ota_finish(https_ota_handle);
        if ((err == ESP_OK) && (ota_finish_err == ESP_OK))
        {
            ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, OTADRIVE_EVENT_FINISH_UPDATE, NULL, 0, portMAX_DELAY));

            ESP_LOGI(TAG, "ESP_HTTPS_OTA upgrade successful. Rebooting ...");
            ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, OTADRIVE_EVENT_PENDING_REBOOT, NULL, 0, portMAX_DELAY));
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            esp_restart();
            return r;
        }
        else
        {
            if (ota_finish_err == ESP_ERR_OTA_VALIDATE_FAILED)
            {
                ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            }
            ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed 0x%x", ota_finish_err);
            ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, OTADRIVE_EVENT_UPDATE_FAILED, NULL, 0, portMAX_DELAY));
            xSemaphoreGive(otadrv_lock);
            return r;
        }
    }

ota_end:
    esp_https_ota_abort(https_ota_handle);
    ESP_LOGE(TAG, "ESP_HTTPS_OTA upgrade failed");
    ESP_ERROR_CHECK(esp_event_post(OTADRIVE_EVENTS, OTADRIVE_EVENT_UPDATE_FAILED, NULL, 0, portMAX_DELAY));
    xSemaphoreGive(otadrv_lock);
    return r;
}

esp_err_t otadrive_rollback()
{
    /*esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (partition == NULL)
    {
        ESP_LOGE(TAG, "Passive OTA partition not found");
        return ESP_FAIL;
    }

    esp_err_t err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        return ESP_FAIL;
    }
    else
    {
        // esp_https_ota_dispatch_event(ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION, (void *)(partition->subtype), sizeof(esp_partition_subtype_t));
        esp_restart();
        return ESP_OK;
    }*/

    return ESP_FAIL;
}

char *otadrive_get_event_str(otadrive_event_e event)
{
    switch (event)
    {
    case OTADRIVE_EVENT_START_CHECK:
        return "OTADRIVE_EVENT_START_CHECK";
    case OTADRIVE_EVENT_UPDATE_AVAILABLE:
        return "OTADRIVE_EVENT_UPDATE_AVAILABLE";
    case OTADRIVE_EVENT_NOUPDATE_AVAILABLE:
        return "OTADRIVE_EVENT_NOUPDATE_AVAILABLE";
    case OTADRIVE_EVENT_START_UPDATE:
        return "OTADRIVE_EVENT_START_UPDATE";
    case OTADRIVE_EVENT_FINISH_UPDATE:
        return "OTADRIVE_EVENT_FINISH_UPDATE";
    case OTADRIVE_EVENT_UPDATE_FAILED:
        return "OTADRIVE_EVENT_UPDATE_FAILED";
    case OTADRIVE_EVENT_START_STORAGE_UPDATE:
        return "OTADRIVE_EVENT_START_STORAGE_UPDATE";
    case OTADRIVE_EVENT_FINISH_STORAGE_UPDATE:
        return "OTADRIVE_EVENT_FINISH_STORAGE_UPDATE";
    case OTADRIVE_EVENT_STORAGE_UPDATE_FAILED:
        return "OTADRIVE_EVENT_STORAGE_UPDATE_FAILED";
    case OTADRIVE_EVENT_FIRMWARE_UPDATE_PROGRESS:
        return "OTADRIVE_EVENT_FIRMWARE_UPDATE_PROGRESS";
    case OTADRIVE_EVENT_STORAGE_UPDATE_PROGRESS:
        return "OTADRIVE_EVENT_STORAGE_UPDATE_PROGRESS";
    case OTADRIVE_EVENT_PENDING_REBOOT:
        return "OTADRIVE_EVENT_PENDING_REBOOT";
    }
    return "Unknown Event";
}

bool otadrive_timeTick(uint16_t seconds)
{
    int64_t tickTimestamp = 0;
    if (esp_timer_get_time() > tickTimestamp)
    {
        tickTimestamp = esp_timer_get_time() + ((uint32_t)seconds) * 1000 * 1000;
        return true;
    }
    return false;
}

char *otadrive_currentversion()
{
    return otadrv_hdl.current_version;
}

char *otadrive_getChipId()
{
    return otadrv_hdl.serial;
}

bool free_buf(char **buf)
{
    if (*buf == NULL)
    {
        free(*buf);
        buf = NULL;
    }
    return true;
}

bool allocate_buf(char **buf, uint32_t size)
{
    free_buf(buf);

    *buf = (char *)calloc(size, sizeof(char));
    if (*buf == NULL)
    {
        ESP_LOGE(TAG, "Memory not allocated.\n");
        return false;
    }
    memset((void *)*buf, 0, size);

    return true;
}