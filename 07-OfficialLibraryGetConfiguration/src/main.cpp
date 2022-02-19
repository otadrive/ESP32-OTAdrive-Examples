#include <Arduino.h>
#include <otadrive_esp.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

typedef struct configs
{
  uint32_t initialized_sign;
  char ssid[33];
  char pass[33];
  uint16_t on_time;
  uint16_t off_time;
};

configs local_conf;

void setup()
{
  // load configs from EEPROM
  EEPROM.begin(512);
  EEPROM.get(0, local_conf);
  if (local_conf.initialized_sign != 0x123456AB)
  {
    // load default configs
    local_conf.initialized_sign = 0x123456AB;
    strcpy(local_conf.ssid, "OTAdrive");
    strcpy(local_conf.pass, "$OTAdrive#");
    local_conf.on_time = 250;
    local_conf.off_time = 250;
    EEPROM.put(0, local_conf);
  }

  OTADRIVE.setInfo("bd076abe-a423-4880-85b3-4367d07c8eda", "1.0.0");
  WiFi.begin(local_conf.ssid, local_conf.pass);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
}

void sync_otadrive()
{
  if (WiFi.status() != WL_CONNECTED)
    return;
  static int sync_timer = 0;
  sync_timer++;
  if (sync_timer < 10)
    return;
  sync_timer = 0;

  // do sync stuff here
  OTADRIVE.updateFirmware();

  String c = OTADRIVE.getConfigs();
  Serial.printf("configs : %s\n", c.c_str());

  // deserialize json configs
  StaticJsonDocument<250> doc;
  if (deserializeJson(doc, c) != DeserializationError::Ok)
  {
    Serial.println("deserialize error");
    return;
  }
  strcpy(local_conf.ssid, doc["ssid"]);
  strcpy(local_conf.pass, doc["pass"]);
  local_conf.on_time = doc["blink_on"];
  local_conf.off_time = doc["blink_off"];
  EEPROM.put(0, local_conf);
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(local_conf.on_time);
  digitalWrite(LED_BUILTIN, LOW);
  delay(local_conf.off_time);

  sync_otadrive();
}