
// Important Notice: Please enable log outputs. Tools->Core Debug Level->Debug

#include <Arduino.h>
#include <otadrive_esp.h>
#include <WiFi.h>

#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@1.2.17"          // this app version
#define LED 2
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
void onUpdateProgress(int progress, int totalt);

void setup()
{
  // Important Notice: Please enable log outputs. Tools->Core Debug Level->Debug
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

  log_i("WiFi connected");
  OTADRIVE.setInfo(APIKEY, FW_VER);
  OTADRIVE.onUpdateFirmwareProgress(onUpdateProgress);
}

void loop()
{
  log_i("Loop: Application version %s", FW_VER);
  if (WiFi.status() == WL_CONNECTED)
  {
    // Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // retrive firmware info from OTAdrive server
      auto inf = OTADRIVE.updateFirmwareInfo();

      // update firmware if newer available
      if (inf.available)
      {
        log_i("\nNew version available, %dBytes, %s\n", inf.size, inf.version.c_str());
        OTADRIVE.updateFirmware();
      }
      else
      {
        log_i("\nNo newer version\n");
      }
    }
  }
  delay(5000);
}

// put function definitions here:
void onUpdateProgress(int progress, int totalt)
{
  static int last = 0;
  int progressPercent = (100 * progress) / totalt;
  Serial.print("*");
  if (last != progressPercent && progressPercent % 10 == 0)
  {
    // print every 10%
    Serial.printf("%d", progressPercent);
  }
  last = progressPercent;
}
