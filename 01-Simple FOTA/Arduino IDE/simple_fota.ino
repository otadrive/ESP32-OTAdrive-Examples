#include <Arduino.h>
#include <otadrive_esp.h>

#define APIKEY "bd076abe-a423-4880-85b3-4367d07c8eda" //"COPY_APIKEY_HERE"
#define FW_VER "v@1.1.135"// "v@x.x.x" // this app version
#define LED 2
#define WIFI_SSID "OTAdrive2"
#define WIFI_PASS "@tadr!ve"

void update();
void onUpdateProgress(int progress, int totalt);

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  log_i("try connect wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    log_i(".");
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(400);
  }

  log_i("WiFi connected");
  OTADRIVE.setInfo(APIKEY, FW_VER);
  OTADRIVE.onUpdateFirmwareProgress(onUpdateProgress);
}

void loop() {
  log_i("loop");
  if (WiFi.status() == WL_CONNECTED) {
    // Every 30 seconds
    if (OTADRIVE.timeTick(30)) {
      // retrive firmware info from OTAdrive server
      updateInfo inf = OTADRIVE.updateFirmwareInfo();
      log_d("\nfirmware info: %s ,%dBytes\n%s\n",
                    inf.version.c_str(), inf.size, inf.available ? "New version available" : "No newer version");
      // update firmware if newer available
      if (inf.available)
        OTADRIVE.updateFirmware();
    }
  }
  delay(5000);
}

void onUpdateProgress(int progress, int totalt) {
  static int last = 0;
  int progressPercent = (100 * progress) / totalt;
  Serial.print("*");
  if (last != progressPercent && progressPercent % 10 == 0) {
    //print every 10%
    Serial.printf("%d", progressPercent);
  }
  last = progressPercent;
}
