#pragma once
#include <Arduino.h>

namespace OTAdrive
{
    class TinyHTTP
    {
        String _url;
        String host;
        String uri;

        bool begin_connect(const String &url);

    public:
        TinyHTTP(Client &c);
        bool get_partial(String url, int partial_st, int partial_len);
        String user_headers;

        uint32_t total_len;
        String MD5;
        bool isPartial;
        Client &client;
    };
}