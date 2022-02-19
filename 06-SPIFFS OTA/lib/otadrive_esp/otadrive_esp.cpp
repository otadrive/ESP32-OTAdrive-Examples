#include "otadrive_esp.h"

void otadrive_ota::setInfo(String ProductKey, String Version)
{
    this->ProductKey = ProductKey;
    this->Version = Version;
}

String otadrive_ota::baseParams()
{
    return "k=" + ProductKey + "&v=" + Version + "&s=" + getChipId();
}

String otadrive_ota::getChipId()
{
#ifdef ESP8266
    return String(ESP.getChipId());
#elif defined(ESP32)
    String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
    ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
    return ChipIdHex;
#endif
}

String otadrive_ota::downloadResourceList()
{
    WiFiClient client;
    HTTPClient http;
    String url = OTADRIVE_URL "resource/list?plain&";
    url += baseParams();

    String res;
    download(url, nullptr, &res);
    return res;
}

String otadrive_ota::head(String url, const char *reqHdrs[1], uint8_t reqHdrsCount)
{
    WiFiClient client;
    HTTPClient http;

    client.setTimeout(TIMEOUT_MS / 1000);
#ifdef ESP32
    http.setConnectTimeout(TIMEOUT_MS);
#endif
    http.setTimeout(TIMEOUT_MS);

    otd_log_i("heading : %s", url.c_str());

    if (http.begin(client, url))
    {
        http.collectHeaders(reqHdrs, reqHdrsCount);
        int httpCode = http.sendRequest("HEAD");

        // httpCode will be negative on error
        if (httpCode == HTTP_CODE_OK)
        {
            String hdrs = "";
            for (uint8_t i = 0; i < http.headers(); i++)
                hdrs += http.headerName(i) + ": " + http.header(i) + "\n";
            return hdrs;
        }
        else
        {
            otd_log_e("downloaded error %d, %s", httpCode, http.errorToString(httpCode).c_str());
        }

        otd_log_i("\n");
    }

    return "";
}

bool otadrive_ota::download(String url, File *file, String *outStr)
{
    WiFiClient client;
    HTTPClient http;

    client.setTimeout(TIMEOUT_MS / 1000);
#ifdef ESP32
    http.setConnectTimeout(TIMEOUT_MS);
#endif
    http.setTimeout(TIMEOUT_MS);

    otd_log_i("downloading : %s", url.c_str());

    if (http.begin(client, url))
    {
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode == HTTP_CODE_OK)
        {
            if (file)
            {
                auto strm = http.getStream();
                int n = 0;
                uint8_t wbuf[256];
                while (strm.available())
                {
                    int rd = strm.readBytes(wbuf, sizeof(wbuf));
                    n += file->write(wbuf, rd);
                }

                otd_log_i("%d bytes downloaded and writted to file : %s", n, file->name());
                return true;
            }

            if (outStr)
            {
                *outStr = http.getString();
                return true;
            }
        }
        else
        {
            otd_log_e("downloaded error %d, %s, %s", httpCode, http.errorToString(httpCode).c_str(), http.getString().c_str());
        }

        otd_log_i("\n");
    }

    return false;
}

bool otadrive_ota::syncResources()
{
    String list = downloadResourceList();
    String baseurl = OTADRIVE_URL "resource/get?";
    baseurl += baseParams();

    while (list.length())
    {
        // extract file info from webAPI
        String fn = cutLine(list);
        String fk = cutLine(list);
        String md5 = cutLine(list);
        if (!fn.startsWith("/"))
            fn = "/" + fn;

        otd_log_d("file data: %s, MD5=%s", fn.c_str(), md5.c_str());

        // check local file MD5 if exists
        String md5_local = "";
        if (fileObj.exists(fn))
        {
            File file = fileObj.open(fn, "r");
            md5_local = file_md5(file);
            file.close();
        }

        // compare local and server file checksum
        if (md5_local == md5)
        {
            otd_log_i("local MD5 is match for %s", fn.c_str());
            continue;
        }
        else
        {
            // lets download and replace file
            otd_log_i("MD5 not match for %s, lets download", fn.c_str());
            File file = fileObj.open(fn, "w+");
            if (!file)
            {
                otd_log_e("Faild to create file\n");
                return false;
            }

            download(baseurl + "&fk=" + fk, &file, nullptr);
            file.close();
        }
    }

    return true;
}

bool otadrive_ota::sendAlive()
{
    String url = OTADRIVE_URL "alive?";
    url += baseParams();
    return download(url, nullptr, nullptr);
}

void otadrive_ota::updateFirmware()
{
    updateInfo inf = updateFirmwareInfo();

    if (!inf.available)
        return;
    
    String url = OTADRIVE_URL "update?";
    url += baseParams();

    WiFiClient client;

    Update.onProgress(onUpdateFirmwareProgress);
    updateObj.rebootOnUpdate(false);
    t_httpUpdate_return ret = updateObj.update(client, url, Version);

    switch (ret)
    {
    case HTTP_UPDATE_FAILED:
        otd_log_i("HTTP_UPDATE_FAILED Error (%d): %s\n", updateObj.getLastError(), updateObj.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        otd_log_i("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
    {
        Version = inf.version;
        sendAlive();
        otd_log_i("HTTP_UPDATE_OK");
        ESP.restart();
        break;
    }
    }
}

void otadrive_ota::onUpdateFirmwareProgress(int progress, int totalt)
{
    static int last = 0;
    int progressPercent = (100 * progress) / totalt;
    otd_log_i("*");
    if (last != progressPercent && progressPercent % 10 == 0)
    {
        //print every 10%
        otd_log_i("%d", progressPercent);
    }
    last = progressPercent;
}

updateInfo otadrive_ota::updateFirmwareInfo()
{
    String url = OTADRIVE_URL "update?";
    url += baseParams();
    const char *reqHdrs[2] = {"X-Version", "Content-Length"};
    String r = head(url, reqHdrs, 2);
    otd_log_i("heads \n%s ", r.c_str());

    updateInfo inf;
    while (r.length())
    {
        String hline = cutLine(r);
        if (hline.startsWith("X-Version"))
        {
            hline.replace("X-Version: ", "");
            inf.version = hline;
        }
        else if (hline.startsWith("Content-Length"))
        {
            hline.replace("Content-Length: ", "");
            inf.size = hline.toInt();
        }
    }

    inf.available = inf.version != Version;

    return inf;
}