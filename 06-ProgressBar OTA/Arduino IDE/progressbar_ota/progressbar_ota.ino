// Important Notice: Please enable log outputs. Tools->Core Debug Level->Debug

#include <otadrive_esp.h>
#include <Arduino.h>
#include <WiFi.h>
// instal zip lib download from here https://otadrive.com/dwnl/TFT_eSPI.zip then go to (sketch->include libraty->add zip)
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();           // Create object "tft"
TFT_eSprite img = TFT_eSprite(&tft); // Create Sprite object "img" with pointer to "tft" object

#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@6.2.8"                              // this app version
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

int counter = 0;

void ota_proggress(size_t downloaded, size_t total);

void printInfo()
{
  log_i("Application version %s, Serial:%s, IP:%s", FW_VER, OTADRIVE.getChipId().c_str(), WiFi.localIP().toString().c_str());
}

void draw_lcd(uint8_t step)
{
  // tft.fillScreen(TFT_BLACK);
  // Resolution: 135 x 240
  tft.setCursor(0, 0);
  tft.setTextSize(3);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.print("App ");
  tft.print(FW_VER);
  tft.drawLine(0, 32, 256, 32, TFT_GREENYELLOW);
  // show Lamps
  tft.fillRect(0, 117-5, 240, 11, TFT_BLACK);
  tft.fillCircle(5 + (10 * (counter % 24)), 117, 5, TFT_GREEN);

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
  // instal zip lib download from here https://otadrive.com/dwnl/TFT_eSPI.zip then go to (sketch->include libraty->add zip)
  start_lcd();
  OTADRIVE.timeTick(30);
  delay(2500);
  Serial.begin(115200);

  printInfo();

  log_i("try connect wifi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    log_i(".");
    draw_lcd(1);
    delay(250);
  }

  log_i("WiFi connected %s", WiFi.localIP().toString().c_str());
  OTADRIVE.setInfo(APIKEY, FW_VER);
  // Initialize callback progress method while FOTA
  OTADRIVE.onUpdateFirmwareProgress(ota_proggress);
}

void loop()
{
  draw_lcd(2);
  printInfo();

  if (WiFi.status() == WL_CONNECTED)
  {
    // Every 180 seconds
    if (OTADRIVE.timeTick(30))
    {
      // You can do something about config and FOTA (Firmware OTA) here
      auto inf = OTADRIVE.updateFirmwareInfo();
      if (inf.available)
      {
        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(0, 40);
        tft.printf("UPDATING\nFrom %s\nTo %s", FW_VER, inf.version);

        OTADRIVE.updateFirmware();
      }
    }
  }

  delay(1000);
}

void ota_proggress(size_t downloaded, size_t total)
{
  int percent = downloaded / (total / 100 + 1);
  tft.setCursor(0, 20);
  tft.printf("%d/%d   %d%%", downloaded, total, percent);
  tft.fillRect(0, 100, percent * 2, 10, TFT_GREEN);
}
