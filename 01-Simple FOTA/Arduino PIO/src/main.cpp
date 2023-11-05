#include <Arduino.h>
#include <otadrive_esp.h>

#define APIKEY "COPY_APIKEY_HERE" // OTAdrive APIkey for this product
#define FW_VER "v@x.x.x" // this app version
#define LED 2
#define WIFI_SSID "OTAdrive2"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
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
  log_i("Loop: Application version %s", FW_VER);
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

// put function definitions here:
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
