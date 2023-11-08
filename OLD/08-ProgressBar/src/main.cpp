#include <Arduino.h>
#include <otadrive_esp.h>

// To inject firmware info into binary file, You have to use following macro according to let
// OTAdrive to detect binary info automatically
#define ProductKey "c0af643b-4f90-4905-9807-db8be5164cde" // Replace with your own APIkey
#define Version "v@1.0.0.6"

void update();
void OnProgress(int progress, int totalt);

void setup()
{
  OTADRIVE.setInfo(ProductKey, Version);
  OTADRIVE.onUpdateFirmwareProgress(OnProgress);
  // dummy delay
  delay(1500);

  Serial.begin(115200);
  Serial.println("OTAdrive.com ESP32 update progress example");
  Serial.print("Chip Serial: ");
  Serial.println(OTADRIVE.getChipId().c_str());
  Serial.print("Firmware version: ");
  Serial.println(Version);

  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("smarthomehub", "smarthome2015");
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(2, 1);
  delay(250);
  digitalWrite(2, 0);
  delay(250);
  Serial.print("*");

  if (WiFi.status() == WL_CONNECTED)
  {
    if (OTADRIVE.timeTick(20))
    {
      Serial.println();
      OTADRIVE.updateFirmware();
    }
  }
}

int last = 0;
void OnProgress(int progress, int totalt)
{
  int progressPercent = (100 * progress) / totalt;
  Serial.print(".");
  if (last != progressPercent && progressPercent % 10 == 0)
  {
    // print every 10%
    Serial.println(progressPercent);
  }
  last = progressPercent;
}
