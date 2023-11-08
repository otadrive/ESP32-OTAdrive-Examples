#include <Arduino.h>
#include <otadrive_esp.h>
#include <WiFi.h>

#define APIKEY "bd076abe-a423-4880-85b3-4367d07c8eda"//"COPY_APIKEY_HERE" // OTAdrive APIkey for this product
#define FW_VER "v@1.2.3"          // this app version
#define LED 2
#define WIFI_SSID "OTAdrive2"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
OTAdrive_ns::KeyValueList configs;
int speed = 50;

void setup()
{
  delay(2500);
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  otd_log_i("Start application. Version %s, Serial: %s", FW_VER, OTADRIVE.getChipId().c_str());

  log_i("try connect wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    log_i(".");
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(400);
  }

  log_i("WiFi connected %s", WiFi.localIP().toString().c_str());
  OTADRIVE.setInfo(APIKEY, FW_VER);
}

void loop()
{
  otd_log_i("Loop: Version %s, Serial: %s", FW_VER, OTADRIVE.getChipId().c_str());
  log_i("config parameter [speed]=%d", speed);

  if (WiFi.status() == WL_CONNECTED)
  {
    // Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // get latest config
      configs = OTADRIVE.getConfigValues();
      if (configs.containsKey("speed"))
      {
        speed = configs.value("speed").toInt();
      }

      // We don't talk about FOTA here. So code removed
    }
  }
  delay(5000);
}
