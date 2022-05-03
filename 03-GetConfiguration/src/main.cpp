#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <otadrive_esp.h>

// configuration variables
int onDelay;
int offDelay;

// To inject firmware info into binary file, You have to use following macro according to let
// OTAdrive to detect binary info automatically
#define ProductKey "c0af643b-4f90-4905-9807-db8be5164cde" // Replace with your own APIkey
#define Version "1.0.0.7"

void update();
void updateConfigs();
void saveConfigs();
void loadConfigs();

void setup()
{
  OTADRIVE.setInfo(ProductKey, Version);
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("OTAdrive", "@tadr!ve");

  EEPROM.begin(32);
  loadConfigs();

  Serial.begin(115200);
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(2, 1);
  delay(onDelay + 20);
  digitalWrite(2, 0);
  delay(offDelay + 20);
  Serial.printf("blink\n");

  if (WiFi.status() == WL_CONNECTED)
  {
    if (OTADRIVE.timeTick(15))
    {
      update();
      updateConfigs();

    }
  }
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
  String payload = OTADRIVE.getConfigs();
  DynamicJsonDocument doc(512);
  deserializeJson(doc, payload);

  Serial.printf("http content: %s\n", payload.c_str());

  if (doc.containsKey("onDelay") &&
      doc.containsKey("offDelay"))
  {
    onDelay = doc["onDelay"].as<int>();
    offDelay = doc["offDelay"].as<int>();
    saveConfigs();
  }
}

void update()
{
  auto r = OTADRIVE.updateFirmware();
  Serial.printf("Update result is: %s\n", r.toString().c_str());
}
