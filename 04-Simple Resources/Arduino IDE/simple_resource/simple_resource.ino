// Important Notice: Please enable log outputs. Tools->Core Debug Level->Debug

#include <Arduino.h>
#include <otadrive_esp.h>
#include <WiFi.h>

#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@31.21.3"          // this app version
#define LED 2
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

// put function declarations here:
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

void printInfo()
{
  log_i("Application version %s, Serial:%s, IP:%s", FW_VER, OTADRIVE.getChipId().c_str(), WiFi.localIP().toString().c_str());
}


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
    digitalWrite(LED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    delay(400);
  }

  log_i("WiFi connected %s", WiFi.localIP().toString().c_str());
  OTADRIVE.setInfo(APIKEY, FW_VER);

  // Initialize file system and set the OTAdrive library file handler
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  OTADRIVE.setFileSystem(&SPIFFS);
}

void loop()
{
  printInfo();

  if (WiFi.status() == WL_CONNECTED)
  {
    // Every 30 seconds
    if (OTADRIVE.timeTick(30))
    {
      // download new files (and rewrite modified files) from the OTAdrive
      OTADRIVE.syncResources();

      // print the list of local files
      listDir(SPIFFS, "/", 0);

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

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname, "r");
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.path());
      Serial.print("\t    SIZE: ");
      Serial.print(file.size());
      Serial.println(" Bytes");
    }
    file = root.openNextFile();
  }
}