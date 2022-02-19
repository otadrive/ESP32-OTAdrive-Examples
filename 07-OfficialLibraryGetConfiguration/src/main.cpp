#include <Arduino.h>
#include <otadrive_esp.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

struct configs
{
  uint32_t sign;
  char ssid[33];
  char pass[33];
  uint16_t blink_on;
  uint16_t blink_off;
};

configs local_configs;

void setup()
{
  OTADRIVE.setInfo("08ef4e5f-f062-40bb-b21d-fd7f13124609", "1.0.0");
  Serial.begin(115200);
  EEPROM.begin(512);
  EEPROM.get(0, local_configs);

  if (local_configs.sign != 0xA123B456)
  {
    Serial.println("Load default configs");
    strcpy(local_configs.ssid, "OTAdrive");
    strcpy(local_configs.pass, "$OTAdrive#");
    local_configs.blink_on = 250;
    local_configs.blink_off = 350;
    local_configs.sign = 0xA123B456;
    EEPROM.put(0, local_configs);
    EEPROM.commit();
  }
  else
  {
    Serial.println("Configs are valid");
  }

  WiFi.begin(local_configs.ssid, local_configs.pass);
  pinMode(LED_BUILTIN, OUTPUT);
}

void sync_task()
{
  static int sync_timer = 0;
  sync_timer++;
  if (sync_timer < 10)
    return;
  sync_timer = 0;

  if (WiFi.status() != WL_CONNECTED)
    return;

  // do sync and update operations here
  OTADRIVE.updateFirmware();

  String c = OTADRIVE.getConfigs();
  Serial.printf("config download: %s\n", c.c_str());
  StaticJsonDocument<250> doc;
  if (deserializeJson(doc, c) != DeserializationError::Ok)
  {
    Serial.println("deserialize error");
    return;
  }

  Serial.printf("configs decode ssid=%s,blink_on=%d\n", (const char *)doc["ssid"], (int)doc["blink_on"]);

  local_configs.blink_on = doc["blink_on"];
  local_configs.blink_off = doc["blink_off"];
  strcpy(local_configs.ssid, doc["ssid"]);
  strcpy(local_configs.pass, doc["pass"]);

  EEPROM.put(0, local_configs);
  EEPROM.commit();
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(local_configs.blink_on);
  digitalWrite(LED_BUILTIN, LOW);
  delay(local_configs.blink_off);
  sync_task();
}