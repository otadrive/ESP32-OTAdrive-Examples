/*
Please define the following configuration in your product and assign it to the group of your device
{
    "speed":180,
    "alarm":{
        "number":"+188554436",
        "msg1":"Fire, please help",
        "msg2":"Emergency, please help"
    }
}
*/


#include <Arduino.h>
#include <otadrive_esp.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@21.3.5"                             // this app version
#define LED 2
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
int speed = 50;
String alarm_num;
String alarm_msg1;

void printInfo()
{
  log_i("Application version %s, Serial:%s, IP:%s", FW_VER, OTADRIVE.getChipId().c_str(), WiFi.localIP().toString().c_str());
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  log_i("try connect wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    log_i(".");
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(400);
  }

  log_i("WiFi connected %s", WiFi.localIP().toString().c_str());
  OTADRIVE.setInfo(APIKEY, FW_VER);
}

void loop()
{
  printInfo();
  log_i("config parameter [speed]=%d,alarm %s -> %s", speed, alarm_num.c_str(), alarm_msg1.c_str());

  if (WiFi.status() == WL_CONNECTED)
  {
    // Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // get latest config
      String payload = OTADRIVE.getJsonConfigs();
      DynamicJsonDocument doc(512);
      deserializeJson(doc, payload);

      // extract values
      if (doc.containsKey("speed"))
        speed = doc["speed"].as<int>();

      if (doc.containsKey("alarm"))
      {
        if (doc["alarm"].containsKey("number"))
          alarm_num = doc["alarm"]["number"].as<String>();

        if (doc["alarm"].containsKey("msg1"))
          alarm_msg1 = doc["alarm"]["msg1"].as<String>();
      }

      // We don't talk about FOTA here. So code removed
      // // retrive firmware info from OTAdrive server
      // updateInfo inf = OTADRIVE.updateFirmwareInfo();
      // log_d("\nfirmware info: %s ,%dBytes\n%s\n",
      //       inf.version.c_str(), inf.size, inf.available ? "New version available" : "No newer version");
      // // update firmware if newer available
      // if (inf.available)
      //   OTADRIVE.updateFirmware();
    }
  }
  delay(5000);
}
