# ESP32 Version Injection
An example for ESP32 firmware injection mechanism OTAdrive

# What is OTAdrive

## Prerequests

This example is part of [OTA drive](https://www.otadrive.com) tutorials, you should follow previeus sections to understand what we explain here.

## Problem

While you upload your firmware on [OTA drive](https://www.otadrive.com) you face with following warning about your bin file.

![Version Injection Mechanism Warning](/doc/vi-warning1.png)

## How to inject version

OTAdrive needs some data about your product and version in your bin file. The simplest way to do that is have a plain string in a fixed format in your bin file.
As you know, in all native languages like (c,c++,etc.) compiler stores strings somewhere in executable output file.
We use this point to inject our productkey and version into plain text in bin file. Just use following code:

``` c
// To inject firmware info into binary file, You have to use following macro according to let
// OTAdrive to detect binary info automatically
#define ProductKey "c0af643b-4f90-4905-9807-db8be5164cde" // Replace with your own APIkey
#define Version "1.0.0.7" // Replace with your own Version number
#define MakeFirmwareInfo(k, v) "&_FirmwareInfo&k=" k "&v=" v "&FirmwareInfo_&"
.
.
.
void update()
{
  String url = "http://otadrive.com/deviceapi/update?";
  url += MakeFirmwareInfo(ProductKey, Version);
  url += "&s=" + getChipId();

  httpUpdate.setLedPin(2);
  WiFiClient client;
  httpUpdate.update(client, url, Version);
}
```
After use this code in your project, you will see problem solved.

![Version Injection Mechanism Warning](/doc/vi-success.png)

## More info
For more information please follow up tutorials in [OTA drive](https://www.otadrive.com)