#include <Arduino.h>
#include <HTTPUpdate.h>

// To inject firmware info into binary file, You have to use following macro according to let
// OTAdrive to detect binary info automatically
#define ProductKey "c0af643b-4f90-4905-9807-db8be5164cde" // Replace with your own APIkey
#define Version "1.0.0.7"
#define MakeFirmwareInfo(k, v) "&_FirmwareInfo&k=" k "&v=" v "&FirmwareInfo_&"

void update();
void setup()
{
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("guest", "GTXci7@@!q");
}

uint32_t updateCounter = 0;

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(2, 1);
  delay(250);
  digitalWrite(2, 0);
  delay(250);

  if (WiFi.status() == WL_CONNECTED)
  {
    updateCounter++;
    if (updateCounter > 20)
    {
      updateCounter = 0;
      update();
    }
  }
}

String getChipId()
{
  String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
  ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
  return ChipIdHex;
}

void update()
{
  String url = "http://otadrive.com/deviceapi/update?";
  url += MakeFirmwareInfo(ProductKey, Version);
  url += "&s=" + getChipId();

  httpUpdate.setLedPin(2);
  WiFiClient client;
  httpUpdate.update(client, url, Version);
}