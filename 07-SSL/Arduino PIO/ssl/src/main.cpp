#include <Arduino.h>
#include <otadrive_esp.h>
#include <WiFiClientSecure.h>
#include <wifi.h>


#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@11.2.3"                             // this app version
#define LED 2
#define WIFI_SSID "OTAdrive2"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
OTAdrive_ns::KeyValueList configs;
int speed = 50;
bool checkPeer(WiFiClientSecure &ssl_client);

void setup()
{
  delay(2500);
  delay(2500);
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  otd_log_i("Start application. Version %s, Serial: %s", FW_VER, OTADRIVE.getChipId().c_str());

  otd_log_i("try connect wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    otd_log_i(".");
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(400);
  }

  otd_log_i("WiFi connected %s", WiFi.localIP().toString().c_str());
  OTADRIVE.setInfo(APIKEY, FW_VER);
  OTADRIVE.setChipId("AABBCCDD");
  // enable SSL secure connection
  // enable SSL secure connection
  OTADRIVE.useSSL(true);
}

void loop()
{
  otd_log_i("Loop: Version %s, Serial: %s", FW_VER, OTADRIVE.getChipId().c_str());
  otd_log_i("config parameter [speed]=%d", speed);
  delay(5000);

  if (WiFi.status() == WL_CONNECTED)
  {
    // Create a secure client and set the OTAdrive certificate to it
    WiFiClientSecure ssl_client;
    ssl_client.setCACert(otadrv_ca);

    // use this check if you really worry about attack and need most security
    if (!checkPeer(ssl_client))
      return;

    //  Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // get latest config
      configs = OTADRIVE.getConfigValues(ssl_client);
      if (configs.containsKey("speed"))
      {
        speed = configs.value("speed").toInt();
      }

      auto inf = OTADRIVE.updateFirmwareInfo(ssl_client);
      if (inf.available)
      {
        // You may need do something befor update
        OTADRIVE.updateFirmware(ssl_client);
      }
    }
  }
}

bool checkPeer(WiFiClientSecure &ssl_client)
{
  // We should check that SSL certificate on the server is belongs to
  // the "otadrive.com" or it may be an fake middle SSL.
  // This make the connection procedure a bit slower
  if (ssl_client.connect("otadrive.com", 443))
  {
    auto peer = ssl_client.getPeerCertificate();
    if (peer == NULL)
    {
      log_e("Faild to get SSL peer");
      return false;
    }

    if (peer->subject.val.p == nullptr)
    {
      log_e("Faild to get correct SSL peer");
      return false;
    }

    if (strncmp((char *)peer->subject.val.p, "*.otadrive.com", 14) == 0 ||
        strncmp((char *)peer->subject.val.p, "otadrive.com", 12) == 0)
    {
      log_i("Certificate valid for otadrive.com");
    }
    else
    {
      log_e("Certificate is not valid %s", peer->subject.val.p);
      return false;
    }
  }
  else
  {
    log_e("Faild to connect SSL peer");
    return false;
  }
  return true;
}