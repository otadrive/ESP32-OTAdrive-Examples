# Why Use SSL for OTA
SSL cryptography will encrypt all communication in the TCP layer of data transfer. It ensures no one can understand what your MCU and server are discussing. It is the most private level you can have with some simple actions.  
The most popular attack in IoT systems is the [man-in-the-middle](https://en.wikipedia.org/wiki/Man-in-the-middle_attack) attack. Everyone can simply do this attack and see the packets your device transmits to the server.
The only thing you can do to stop this is encrypt the data and SSL encryption is the most available method.  

## Disadvantages of SSL
Using SSL requires a lot of additional processes to encrypt and decrypt the packets. Your MCU has a limited RAM resource and low clock frequency, so using SSL is not recommended when you are not worried about an attack.

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