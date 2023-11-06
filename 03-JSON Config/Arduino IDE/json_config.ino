#include <Arduino.h>
#include <otadrive_esp.h>
#include <ArduinoJson.h>


#define APIKEY "COPY_APIKEY_HERE" // OTAdrive APIkey for this product
#define FW_VER "v@x.x.x" // this app version
#define LED 2
#define WIFI_SSID "OTAdrive2"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
int speed = 50;

void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

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
  log_i("Loop: Application version %s", FW_VER);
  log_i("config parameter [speed]=%d", speed);

  if (WiFi.status() == WL_CONNECTED)
  {
    // Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // get latest config
      String payload = OTADRIVE.getJsonConfigs();
      DynamicJsonDocument doc(512);
      deserializeJson(doc, payload);
      if (doc.containsKey("speed"))
      {
        speed = doc["speed"].as<int>();
      }

      // We don't talk about FOTA here. So code removed
    }
  }
  delay(5000);
}
