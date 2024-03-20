# ESP32 Simple OTA/FOTA Example
You can find the example [here](https://github.com/otadrive/ESP32-OTAdrive-Examples/tree/master/01-Simple%20FOTA) or [download](
https://github.com/otadrive/ESP32-OTAdrive-Examples/archive/refs/heads/master.zip) full examples files.  

This example shows the minimum code to use the OTAdrive in your application.
You should initialize the OTAdrive library by calling `setInfo` in the setup menu. Copy the `API key` from your product `overview` tab and replace it in the code. Replace some numbers in version code (`v@2.10.33` ex.) then compile and download the code to the MCU via USB. Then `unplug` the USB, change the code upload the `.bin` file to the OTAdrive and power on the mcu, then see the output.  

You can go ahead via [Quick-start guide](/user/doc/Quick%20Start) for more info.

You can also watch the following video for start.
![www.youtube.com](https://www.youtube.com/embed/jBeAmVS7wtI)

```cpp
#include <otadrive_esp.h>

// OTAdrive APIkey for this product
#define APIKEY "COPY_APIKEY_HERE"
// this app version
#define FW_VER "v@x.x.x"
...
void setup()
{
    ...
    OTADRIVE.setInfo(APIKEY, FW_VER);
    OTADRIVE.onUpdateFirmwareProgress(onUpdateProgress);
    ...
}
...
void loop()
{
    // Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // retrive firmware info from OTAdrive server
      auto inf = OTADRIVE.updateFirmwareInfo();

      // update firmware if newer available
      if (inf.available)
      {
        log_d("\nNew version available, %dBytes, %s\n", inf.size, inf.version.c_str());
        OTADRIVE.updateFirmware();
      }
      else
      {
        log_d("\nNo newer version\n");
      }
    }
}
```