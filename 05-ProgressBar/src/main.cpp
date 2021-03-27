#include <Arduino.h>
#include <HTTPUpdate.h>

// To inject firmware info into binary file, You have to use following macro according to let
// OTAdrive to detect binary info automatically
#define ProductKey "c0af643b-4f90-4905-9807-db8be5164cde" // Replace with your own APIkey
#define Version "1.0.0.6"
#define MakeFirmwareInfo(k, v) "&_FirmwareInfo&k=" k "&v=" v "&FirmwareInfo_&"

void update();
String getChipId();
void OnProgress(int progress, int totalt);

void setup()
{
  // dummy delay
  delay(1500);

  Serial.begin(115200);
  Serial.println("OTAdrive.com ESP32 update progress example");
  Serial.print("Chip Serial: ");
  Serial.println(getChipId().c_str());
  Serial.print("Firmware version: ");
  Serial.println(Version);

  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("smarthomehub", "smarthome2015");
}

uint32_t updateCounter = 0;

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
    updateCounter++;
    if (updateCounter > 20)
    {
      updateCounter = 0;
      Serial.println();
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
  Serial.println("Lets get update");

  // generate api url with version injection mechanism
  String url = "http://otadrive.com/deviceapi/update?";
  url += MakeFirmwareInfo(ProductKey, Version);
  url += "&s=" + getChipId();

  // set update progress callback
  Update.onProgress(OnProgress);

  httpUpdate.setLedPin(2);
  WiFiClient client;
  t_httpUpdate_return ret = httpUpdate.update(client, url, Version);

  // if we reach here, we failed to update
  Serial.print("Update failed with code: ");
  Serial.println(ret);
}

int last = 0;
void OnProgress(int progress, int totalt) {
	int progressPercent = (100 * progress) / totalt;
  Serial.print(".");
	if (last != progressPercent && progressPercent % 10 == 0) {
		//print every 10%
		Serial.println(progressPercent);
	}
	last = progressPercent;
}
