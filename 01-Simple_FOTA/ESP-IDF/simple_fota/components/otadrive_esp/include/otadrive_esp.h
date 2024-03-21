#pragma once

#include <esp_err.h>
#include <esp_event.h>

#define CONFIG_OTADRIVE_VER_LEN 32

#ifdef __cplusplus
extern "C"
{
#endif

    ESP_EVENT_DECLARE_BASE(OTADRIVE_EVENTS);

    /**
     * @brief events
     * These events are posted to the event loop to track progress of the OTA process
     */
    typedef enum
    {
        OTADRIVE_EVENT_START_CHECK = 0x01,               /*!< check started */
        OTADRIVE_EVENT_UPDATE_AVAILABLE = 0x02,          /*!< update available */
        OTADRIVE_EVENT_NOUPDATE_AVAILABLE = 0x04,        /*!< no update available */
        OTADRIVE_EVENT_START_UPDATE = 0x08,              /*!< update started */
        OTADRIVE_EVENT_FINISH_UPDATE = 0x10,             /*!< update finished */
        OTADRIVE_EVENT_UPDATE_FAILED = 0x20,             /*!< update failed */
        OTADRIVE_EVENT_START_STORAGE_UPDATE = 0x40,      /*!< storage update started. If the storage is mounted, you should unmount it when getting this call */
        OTADRIVE_EVENT_FINISH_STORAGE_UPDATE = 0x80,     /*!< storage update finished. You can mount the new storage after getting this call if needed */
        OTADRIVE_EVENT_STORAGE_UPDATE_FAILED = 0x100,    /*!< storage update failed */
        OTADRIVE_EVENT_FIRMWARE_UPDATE_PROGRESS = 0x200, /*!< firmware update progress */
        OTADRIVE_EVENT_STORAGE_UPDATE_PROGRESS = 0x400,  /*!< storage update progress */
        OTADRIVE_EVENT_PENDING_REBOOT = 0x800,           /*!< pending reboot */
    } otadrive_event_e;

    typedef enum
    {
        OTADRIVE_ConnectError = 0,
        OTADRIVE_Success = 1,
        OTADRIVE_DeviceUnauthorized = 401,
        OTADRIVE_AlreadyUpToDate = 304,
        OTADRIVE_NoFirmwareExists = 404,
        OTADRIVE_NewFirmwareExists = 200
    } otadrive_result_code;

    typedef struct otadrive_result
    {
        otadrive_result_code code;
        char available_version[CONFIG_OTADRIVE_VER_LEN];
        int32_t available_size;
    } otadrive_result;

    void otadrive_setInfo(char *apiKey, char *current_version);
    otadrive_result otadrive_updateFirmwareInfo();
    otadrive_result otadrive_updateFirmware();
    char *otadrive_currentversion();
    bool otadrive_timeTick(uint16_t seconds);

    /**
     * @brief convience function to return a string representation of events emited by this library
     *
     * @param event the eventid passed to the event handler
     * @return char* a string representing the event
     */
    char *otadrive_get_event_str(otadrive_event_e event);

#ifdef __cplusplus
}
#endif
