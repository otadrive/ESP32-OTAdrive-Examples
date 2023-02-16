#include <Arduino.h>
#include <TFT_eSPI.h>
#include <otadrive_esp.h>

TFT_eSPI tft = TFT_eSPI();           // Create object "tft"
TFT_eSprite img = TFT_eSprite(&tft); // Create Sprite object "img" with pointer to "tft" object

void ota_proggress(size_t downloaded, size_t total);

void setup()
{
  Serial.begin(115200);
  
  OTADRIVE.setInfo("bd076abe-a423-4880-85b3-4367d07c8eda", "v@3.0.0");
  OTADRIVE.onUpdateFirmwareProgress(ota_proggress);

  WiFi.begin("OTAdrive", "@tadr!ve");
  tft.init();
  tft.setRotation(0);

  tft.fillScreen(TFT_BLACK);
  delay(100);
  tft.setRotation(3);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.print("   Test App v3.0.0");
  tft.drawLine(0, 16, 256, 16, TFT_YELLOW);
  delay(3000);
}

void loop()
{
  tft.fillRect(70, 50, 100, 50, TFT_BLUE);
  delay(200);
  tft.fillRect(70, 50, 100, 50, TFT_WHITE);
  delay(200);
  tft.fillRect(70, 50, 100, 50, TFT_DARKGREEN);
  delay(200);

  if (OTADRIVE.timeTick(30))
  {
    auto inf = OTADRIVE.updateFirmwareInfo();
    if (inf.available)
    {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.printf("Downloading v%s", inf.version.c_str());
      OTADRIVE.updateFirmware();
    }
  }
}

void ota_proggress(size_t downloaded, size_t total)
{
  int percent = downloaded / (total / 100 + 1);
  tft.setCursor(0, 20);
  tft.printf("%d/%d   %d%%", downloaded, total, percent);
  tft.fillRect(0, 56, percent * 2, 10, TFT_GREEN);
}
