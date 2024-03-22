#ifndef _OTD_PRIV_H_
#define _OTD_PRIV_H_

#include <stdint.h>
#include "otadrive_esp.h"
#include <esp_partition.h>
#include "otd_common.h"

static const char *TAG = "OTADRIVE";



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
    esp_partition_t *running_partition;
} otadrive_session_t;
typedef struct otadrive_session_t otadrive_session_t;

extern otadrive_session_t otadrv_hdl;
extern SemaphoreHandle_t otadrv_lock;
extern esp_err_t otd_http_event_handler(esp_http_client_event_t *evt);
extern bool free_buf(char **buf);
extern bool allocate_buf(char **buf, uint32_t size);
extern char *getSketchMD5String();
extern uint8_t *getSketchMD5();
extern uint32_t sketchSize(bool free);
extern uint32_t getFreeSketchSpace();
extern void fill_request_headers(esp_http_client_handle_t client);

#endif
