#ifndef _OTD_COMMON_H_
#define _OTD_COMMON_H_

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
#include <esp_timer.h>


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



#endif
