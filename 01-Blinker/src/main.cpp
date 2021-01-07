#include <Arduino.h>
#include <HTTPUpdate.h>

void update();
void setup()
{
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("smarthomehub", "*****");
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
  url += "k=c0af643b-4f90-4905-9807-db8be5164cde";
  url += "&v=1.0.0.1";
  url += "&s=" + getChipId();

  WiFiClient client;
  httpUpdate.update(client, url, "1.0.0.1");
}