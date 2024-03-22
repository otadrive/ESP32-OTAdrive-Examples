#include "otd_common.h"
#include "otadrive_esp.h"
#include "otd_privates.h"

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

uint32_t getFreeSketchSpace()
{
    const esp_partition_t *_partition = esp_ota_get_next_update_partition(NULL);
    if (!_partition)
    {
        return 0;
    }

    return _partition->size;
}

uint32_t sketchSize(bool free)
{
    esp_image_metadata_t data;
    const esp_partition_t *running = otadrv_hdl.running_partition;
    if (!running)
        return 0;
    const esp_partition_pos_t running_pos = {
        .offset = running->address,
        .size = running->size,
    };
    data.start_addr = running_pos.offset;
    esp_image_verify(ESP_IMAGE_VERIFY, &running_pos, &data);
    if (free)
    {
        return running_pos.size - data.image_len;
    }
    else
    {
        return data.image_len;
    }
}

uint8_t *getSketchMD5()
{
    static uint8_t result[16];
    static bool result_valid = false;
    if (result_valid)
    {
        return result;
    }

    const esp_partition_t *running = otadrv_hdl.running_partition;
    if (!running)
    {
        ESP_LOGE(TAG, "Partition could not be found");
        return NULL;
    }

    // lets calculate MD5
    md5_context_t md5_context;
    esp_md5_init(&md5_context);

    uint8_t buf[1024];
    uint32_t remain = sketchSize(false);
    uint32_t i = 0;
    ESP_LOGE(TAG, "MD5 start size %lu", remain);
    while (remain)
    {
        uint32_t readSize = sizeof(buf);
        if (readSize > remain)
            readSize = remain;

        if (esp_partition_read(running, i, buf, readSize) == ESP_OK)
        {
            esp_md5_update(&md5_context, buf, readSize);
        }
        else
        {
            return NULL;
        }

        remain -= readSize;
        i += readSize;
    }
    if (esp_md5_finish(&md5_context, result) == ESP_OK)
    {
        result_valid = true;
        return result;
    }

    return NULL;
}

char *getSketchMD5String()
{
    static char result[33];
    static bool result_valid = false;
    if (result_valid)
    {
        return result;
    }
    result[0] = '\0';

    uint8_t *r = getSketchMD5();
    if (r == NULL)
        return result;
    result_valid = true;

    for (int i = 0; i < 16; i++)
    {
        sprintf(result + (i * 2), "%02X", r[i]);
    }
    result[32] = '\0';
    return result;
}