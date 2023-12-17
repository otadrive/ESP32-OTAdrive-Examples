
# How to enable SSL for OTAdrive
First of all, you should enable SSL for the whole library. Call the `useSSL()` method in the `setup()` function same as the below code:
```cpp
void setup()
{
    ...
    OTADRIVE.setInfo(APIKEY, FW_VER);
    // enable SSL secure connection
    OTADRIVE.useSSL(true);
}
```

Then you should create a secure client object for the methods and pass it to them, do it like the below code:
```cpp
{
    ...
    WiFiClientSecure client;
    client.setCACert(otadrv_ca);

    OTADRIVE.getConfigValues(client);
    OTADRIVE.updateFirmwareInfo(client);
    OTADRIVE.updateFirmware(client);
    ...
}
```