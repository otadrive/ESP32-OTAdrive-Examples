#ifndef _OTADRIVE_ESP_H_
#define _OTADRIVE_ESP_H_

#include <Arduino.h>
#include <FS.h>

#ifdef ESP8266
#include <ESP8266httpUpdate.h>
#include <LittleFS.h>
#define updateObj ESPhttpUpdate
#define fileObj LittleFS
#elif defined(ESP32)
#include <HTTPUpdate.h>
#include <SPIFFS.h>
#define updateObj httpUpdate
#define fileObj SPIFFS
#endif

#define OTADRIVE_URL "http://192.168.1.15/deviceapi/"

class updateInfo
{
public:
    bool available;
    String version;
    int size;
};

class otadrive_ota
{
private:
    const uint16_t TIMEOUT_MS = 10000;
    String ProductKey;
    String Version;

    String cutLine(String &str);
    String baseParams();
    bool download(String url, File *file, String *outStr);
    String head(String url, const char *reqHdrs[1], uint8_t reqHdrsCount);
    String file_md5(File &f);
    String downloadResourceList();

    static void onUpdateFirmwareProgress(int progress, int totalt);

public:
    void setInfo(String ProductKey, String Version);
    String getChipId();

    bool sendAlive();
    void updateFirmware();
    updateInfo updateFirmwareInfo();
    bool syncResources();
};

#ifdef ESP8266
#define otd_log_d(format, ...) Serial.printf(format "\n", ##__VA_ARGS__)
#define otd_log_i(format, ...) Serial.printf(format "\n", ##__VA_ARGS__)
#define otd_log_e(format, ...) Serial.printf(format "\n", ##__VA_ARGS__)
#elif defined(ESP32)
#define otd_log_d(format, ...) log_d(format, ##__VA_ARGS__)
#define otd_log_i(format, ...) log_i(format, ##__VA_ARGS__)
#define otd_log_e(format, ...) log_e(format, ##__VA_ARGS__)
#endif

#endif
