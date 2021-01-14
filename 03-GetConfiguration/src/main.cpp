#include <Arduino.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// configuration variables
int onDelay;
int offDelay;

// To inject firmware info into binary file, You have to use following macro according to let
// OTAdrive to detect binary info automatically
#define ProductKey "c0af643b-4f90-4905-9807-db8be5164cde" // Replace with your own APIkey
#define Version "1.0.0.7"
#define MakeFirmwareInfo(k, v) "&_FirmwareInfo&k=" k "&v=" v "&FirmwareInfo_&"

void update();
void updateConfigs();
void saveConfigs();
void loadConfigs();

void setup()
{
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("smarthomehub", "****");

  EEPROM.begin(32);
  loadConfigs();

  Serial.begin(115200);
}

uint32_t updateCounter = 0;

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(2, 1);
  delay(onDelay);
  digitalWrite(2, 0);
  delay(offDelay);

  if (WiFi.status() == WL_CONNECTED)
  {
    updateCounter++;
    if (updateCounter > 20)
    {
      updateCounter = 0;
      update();
      updateConfigs();
    }
  }
}

String getChipId()
{
  String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
  ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
  return ChipIdHex;
}

void saveConfigs()
{
  EEPROM.writeInt(4, onDelay);
  EEPROM.writeInt(8, offDelay);
  EEPROM.commit();
}

void loadConfigs()
{
  // check configs initialized in eeprom or not
  if (EEPROM.readInt(0) != 0x4A)
  {
    // configs not initialized yet. write for first time
    EEPROM.writeInt(0, 0x4A); // memory sign
    EEPROM.writeInt(4, 200);  // onDelay
    EEPROM.writeInt(8, 250);  // offDelay
    EEPROM.commit();
  }
  else
  {
    // configs initialized and valid. read values
    onDelay = EEPROM.readInt(4);
    offDelay = EEPROM.readInt(8);
  }
}

void updateConfigs()
{
  WiFiClient client;
  HTTPClient http;
  String url = "http://www.otadrive.com/deviceapi/getconfig?";
  url += MakeFirmwareInfo(ProductKey, Version);
  url += "&s=" + getChipId();

  Serial.println(url);

  if (http.begin(client, url))
  {
    int httpCode = http.GET();
    Serial.printf("http code: %d", httpCode);

    // httpCode will be negative on error
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      DynamicJsonDocument doc(512);
      deserializeJson(doc, payload);

      Serial.printf("http content: %s", payload.c_str());

      if (doc.containsKey("onDelay") &&
          doc.containsKey("offDelay"))
      {
        onDelay = doc["onDelay"].as<int>();
        offDelay = doc["offDelay"].as<int>();
        saveConfigs();
      }
    }
  }
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
