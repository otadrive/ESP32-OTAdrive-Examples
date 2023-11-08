#include <Arduino.h>
#include <otadrive_esp.h>

#if ESP32
#include <wifi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#define APIKEY "bd076abe-a423-4880-85b3-4367d07c8eda" //"COPY_APIKEY_HERE" // OTAdrive APIkey for this product
#define FW_VER "v@1.2.3"                              // this app version
#define LED 2
#define WIFI_SSID "OTAdrive2"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
OTAdrive_ns::KeyValueList configs;
int speed = 50;
void setNtpClock();

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
  OTADRIVE.useSSL(true);
  setNtpClock();
}

void loop()
{
  otd_log_i("Loop: Version %s, Serial: %s", FW_VER, OTADRIVE.getChipId().c_str());
  otd_log_i("config parameter [speed]=%d", speed);

  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure client;
    X509List certs;
    certs.append(otadrv_ca);
    char buf[500];
    buf[0] = '\0';
    //client.setInsecure();
    client.setTrustAnchors(&certs);
    
    
    // BA49C09D128076176AEBD53CA3BC3438F03815DF
    // if (client.setFingerprint("07E032E020B72C3F192F0628A2593A19A70F069E"))
    // otd_log_i("fingerprint accept");
    // client.setCACert(otadrv_ca);
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

      client.getLastSSLError(buf, 500);
      otd_log_d("SSL %s", buf);
    }
  }
  delay(5000);
}

// Set time via NTP, as required for x.509 validation
void setNtpClock()
{
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}