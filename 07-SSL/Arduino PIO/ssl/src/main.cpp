#include <Arduino.h>
#include <otadrive_esp.h>

#define APIKEY "bd076abe-a423-4880-85b3-4367d07c8eda" //"COPY_APIKEY_HERE" // OTAdrive APIkey for this product
#define FW_VER "v@1.2.3"                              // this app version
#define LED 2
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
OTAdrive_ns::KeyValueList configs;
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
  OTADRIVE.useSSL(true);
}

void loop()
{
  log_i("Loop: Application version %s", FW_VER);
  log_i("config parameter [speed]=%d", speed);

  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure client;
    client.setCACert(otadrv_ca);
    // Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // get latest config
      configs = OTADRIVE.getConfigValues(client);
      if (configs.containsKey("speed"))
      {
        speed = configs.value("speed").toInt();
      }

      auto inf = OTADRIVE.updateFirmwareInfo(client);
      if (inf.available)
      {
        // You may need do something befor update
        OTADRIVE.updateFirmware(client);
      }
    }
  }
  delay(5000);
}
