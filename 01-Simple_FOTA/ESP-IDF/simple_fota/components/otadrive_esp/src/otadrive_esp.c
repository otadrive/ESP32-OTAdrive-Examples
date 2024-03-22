#include "otd_common.h"
#include "otadrive_esp.h"
#include "otd_privates.h"
#include "esp_mac.h"

ESP_EVENT_DEFINE_BASE(OTADRIVE_EVENTS);

otadrive_session_t otadrv_hdl;
SemaphoreHandle_t otadrv_lock = NULL;

/*String otadrive_ota::getChipId()
{
    String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
    ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
    return ChipIdHex;
}*/

#include <esp_partition.h>
#include <esp_rom_spiflash.h>

void otadrive_setInfo(char *apiKey, char *current_version)
{
    ESP_LOGE(TAG, "MD5 at %s", getSketchMD5String());

    if (!otadrv_lock)
        otadrv_lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(otadrv_lock, pdMS_TO_TICKS(1000)) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to take lock");
        return;
    }

    ESP_LOGI(TAG, "Initializing");
    bzero((void *)&otadrv_hdl, sizeof(otadrive_session_t));
    otadrv_hdl.running_partition = esp_ota_get_running_partition();
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
        .event_handler = otd_http_event_handler,
    };

    esp_http_client_handle_t client = esp_http_client_init(&httpconfig);
    fill_request_headers(client);
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
