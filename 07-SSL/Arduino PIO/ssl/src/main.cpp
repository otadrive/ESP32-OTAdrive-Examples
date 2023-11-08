#include <Arduino.h>
#include <otadrive_esp.h>
#include <WiFiClientSecure.h>
#include <wifi.h>


#define APIKEY "COPY_APIKEY_HERE" // OTAdrive APIkey for this product
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

  otd_log_i("try connect wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    otd_log_i(".");
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(400);
  }

  otd_log_i("WiFi connected %s", WiFi.localIP().toString().c_str());
  OTADRIVE.setInfo(APIKEY, FW_VER);
  // enable SSL secure connection
  OTADRIVE.useSSL(true);
}

void loop()
{
  otd_log_i("Loop: Version %s, Serial: %s", FW_VER, OTADRIVE.getChipId().c_str());
  otd_log_i("config parameter [speed]=%d", speed);

  if (WiFi.status() == WL_CONNECTED)
  {
    // Create a secure client and set the OTAdrive certificate to it
    WiFiClientSecure client;
    client.setCACert(otadrv_ca);
    
    //  Every 30 seconds
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
