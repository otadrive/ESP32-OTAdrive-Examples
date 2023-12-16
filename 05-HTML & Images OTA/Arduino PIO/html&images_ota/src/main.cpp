#include <otadrive_esp.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

#define APIKEY "bd076abe-a423-4880-85b3-4367d07c8eda" // OTAdrive APIkey for this product
#define FW_VER "v@1.2.7"                              // this app version
#define LED 2
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

#define BTN1 13
#define BTN2 12

#define LED_R 19
#define LED_G 17

AsyncWebServer server(80);
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

void printInfo()
{
  log_i("Application version %s, Serial:%s, IP:%s", FW_VER, OTADRIVE.getChipId().c_str(), WiFi.localIP().toString().c_str());
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void server_get_home(AsyncWebServerRequest *request)
{
  Serial.printf("%s\n", request->url().c_str());
  if (request->hasArg("cmd"))
  {
    String command = request->arg("cmd");
    Serial.printf("command %s\n", command.c_str());

    if (command.equals("on_r"))
      digitalWrite(LED_R, HIGH);
    else if (command.equals("off_r"))
      digitalWrite(LED_R, LOW);
    else if (command.equals("on_g"))
      digitalWrite(LED_G, HIGH);
    else if (command.equals("off_g"))
      digitalWrite(LED_G, LOW);
  }

  request->send(SPIFFS, "/static_html/index.html", "text/html");
}

void setup_server()
{
  server.addHandler(new SPIFFSEditor(SPIFFS));
  server.on("/", HTTP_GET, server_get_home);
  server.serveStatic("/", SPIFFS, "/static_html/").setDefaultFile("index.html");
  server.onNotFound(notFound);

  server.begin();
}

void setup()
{
  delay(2500);
  Serial.begin(115200);
  // setup LED's
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);

  printInfo();

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

  // Initialize file system and set the OTAdrive library file handler
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  OTADRIVE.setFileSystem(&SPIFFS);

  // run local HTTP server
  setup_server();
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

      // You can do something about config and FOTA (Firmware OTA) here
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
