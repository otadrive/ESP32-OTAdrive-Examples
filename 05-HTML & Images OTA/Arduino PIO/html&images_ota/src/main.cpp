#include <otadrive_esp.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();           // Create object "tft"
TFT_eSprite img = TFT_eSprite(&tft); // Create Sprite object "img" with pointer to "tft" object

#define APIKEY "bd076abe-a423-4880-85b3-4367d07c8eda" // OTAdrive APIkey for this product
#define FW_VER "v@1.2.8"                              // this app version
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

#define BTN1 0
#define BTN2 35

#define LED_R 21
#define LED_G 12

int counter = 0;
bool RedFlag = false;
bool GreenFlag = false;

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
      RedFlag = true;
    else if (command.equals("off_r"))
      RedFlag = false;
    else if (command.equals("on_g"))
      GreenFlag = true;
    else if (command.equals("off_g"))
      GreenFlag = false;
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

void draw_lcd(uint8_t step)
{
  tft.fillScreen(TFT_BLACK);
  // Resolution: 135 x 240
  tft.setCursor(0, 0);
  tft.setTextSize(3);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.print("App ");
  tft.print(FW_VER);
  tft.drawLine(0, 32, 256, 32, TFT_GREENYELLOW);
  // show Lamps
  if (GreenFlag)
    tft.fillCircle(74, 117, 17, TFT_GREEN);
  else
    tft.drawCircle(74, 117, 17, TFT_GREEN);

  if (RedFlag)
    tft.fillCircle(166, 117, 17, TFT_RED);
  else
    tft.drawCircle(166, 117, 17, TFT_RED);

  // show message
  if (step == 0)
  {
    tft.setCursor(0, 44);
    tft.print("Welcome");
  }
  else if (step == 1)
  {
    tft.setCursor(0, 44);
    tft.printf("WiFi %5d", ++counter);
  }
  else if (step == 2)
  {
    tft.setCursor(0, 44);
    tft.printf("CNT  %5d", ++counter);
  }

  tft.setTextSize(1);
  tft.setCursor(0, 80);
  tft.printf("IP: %s", WiFi.localIP().toString().c_str());
}

void start_lcd()
{
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  draw_lcd(0);
}

void setup()
{
  start_lcd();
  delay(2500);
  Serial.begin(115200);
  // setup LED's
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);

  printInfo();

  log_i("try connect wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    log_i(".");
    draw_lcd(1);
    // digitalWrite(2, HIGH);
    delay(100);
    // digitalWrite(2, LOW);
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
  draw_lcd(2);
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
	  /*auto inf = OTADRIVE.updateFirmwareInfo();
      if (inf.available)
      {
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(0, 40);
        tft.printf("UPDATING\nFrom %s\nTo %s", FW_VER, inf.version);

        OTADRIVE.updateFirmware();
      }*/
    }
  }

  // set lamps
  if (RedFlag)
    digitalWrite(LED_R, HIGH);
  else
    digitalWrite(LED_R, LOW);
  if (GreenFlag)
    digitalWrite(LED_G, HIGH);
  else
    digitalWrite(LED_G, LOW);

  // read buttons
  if (digitalRead(BTN1) == 0)
    GreenFlag = !GreenFlag;
  if (digitalRead(BTN2) == 0)
    RedFlag = !RedFlag;

  delay(500);
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
      Serial.printf("  DIR : %s\n", file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.printf("  FILE: %s\t    SIZE: %d Bytes\n", file.path(), file.size());
    }
    file = root.openNextFile();
  }
}
