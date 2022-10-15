#include "tinyHTTP.h"
#include <HTTPClient.h>

using namespace OTAdrive;

#define REQ_PARTIAL_LEN (1024 * 32)

TinyHTTP::TinyHTTP(Client &c) : client(c)
{
}

bool TinyHTTP::begin_connect(const String &url)
{
    String u = url;
    // TODO: HTTPS Not supported yet
    if (u.startsWith("https://"))
    {
        return false;
        u.remove(0, 8);
    }
    else if (u.startsWith("http://"))
        u.remove(0, 7);

    //
    host = u.substring(0, u.indexOf('/'));
    uri = u.substring(host.length());
    // TODO: Port may be diffrent than 80

    client.setTimeout(10000);
    // client.stop();
    if (!client.connect(host.c_str(), 80))
    {
        Serial2.printf("connect to %s faild", host.c_str());
        return false;
    }

    return true;
}

bool TinyHTTP::get_partial(String url, int partial_st, int partial_len)
{
    if (url.isEmpty())
        url = _url;
    else
        _url = url;

    if (!begin_connect(url))
        return false;

    String http_get = "GET " + uri + " HTTP/1.1";
    String http_hdr;
    total_len = 0;

    client.setTimeout(30 * 1000);
    // use HTTP/1.0 for update since the update handler not support any transfer Encoding
    http_hdr += "\nHost: " + host;
    http_hdr += "\nConnection: Keep-Alive";
    http_hdr += user_headers;

    // const char *headerkeys[] = {"x-MD5"};
    // size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);

    String xmd5;

    String total_req = http_get +
                       http_hdr +
                       "\nRange: bytes=" + String(partial_st) + "-" + String((partial_st + partial_len) - 1) +
                       "\n\n";
    client.print(total_req.c_str());
    client.flush();

    int resp_code, content_len = 0;

    String resp = client.readStringUntil('\n');
    Serial2.printf("the h %s\n", resp.c_str());
    // HTTP/1.1 401 Unauthorized
    // HTTP/1.1 200 OK
    if (!resp.startsWith("HTTP/1.1 "))
        return false;
    resp_code = resp.substring(9, 9 + 3).toInt();

    if (resp_code != HTTP_CODE_OK && resp_code != HTTP_CODE_PARTIAL_CONTENT)
    {
        log_e("HTTP error: %d,%s\n", resp_code, resp.c_str());
        return false;
    }
    isPartial = (resp_code == HTTP_CODE_PARTIAL_CONTENT);

    // response headers
    do
    {
        resp = client.readStringUntil('\n');
        resp.trim();
        Serial2.printf("hdr %s\n", resp.c_str());
        if (resp.startsWith("Content-Length"))
        {
            content_len = resp.substring(16).toInt();
        }
        else if (resp.startsWith("x-MD5"))
        {
            xmd5 = resp.substring(7);
        }
        else if (resp.startsWith("Content-Range: bytes"))
        {
            // Content-Range: bytes 0-51199/939504
            int total_ind = resp.indexOf('/');
            total_len = resp.substring(total_ind + 1).toInt();
        }

        if (resp.isEmpty())
            break;
    } while (true);

    if (total_len == 0 && !isPartial)
        return false;
    if (total_len == 0)
        total_len = content_len;

    return true;
}
