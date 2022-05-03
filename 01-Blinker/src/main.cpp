#include <Arduino.h>
#include <otadrive_esp.h>

void setup()
{
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  OTADRIVE.setInfo("c0af643b-4f90-4905-9807-db8be5164cde", "1.0.0.1");
  WiFi.begin("OTAdrive", "@tadr!ve");
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(2, 1);
  delay(250);
  digitalWrite(2, 0);
  delay(250);

  if (WiFi.status() == WL_CONNECTED)
  {
    if (OTADRIVE.timeTick(10))
    {
      OTADRIVE.updateFirmware();
    }
  }
}
