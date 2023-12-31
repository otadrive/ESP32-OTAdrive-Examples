#include <otadrive_esp.h>
#include <Arduino.h>
// install TinyGSM library (v0.11.7 or newer)
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

#include <TFT_eSPI.h>

#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@8.5.2"                             // this app version

// GSM Modem Serial interface
HardwareSerial mySerial(1);
// AT Command channel
TinyGsm modem(mySerial);
// MuxChannel 0 for MQTT
TinyGsmClient gsm_mqtt_client(modem, 0);
// MuxChannel 1 for OTA
TinyGsmClient gsm_otadrive_client(modem, 1);

// Display
TFT_eSPI tft = TFT_eSPI();           // Create object "tft"
TFT_eSprite img = TFT_eSprite(&tft); // Create Sprite object "img" with pointer to "tft" object

String gprs_ip;
int counter = 0;

void printInfo()
{
  Serial.printf("Example08: OTA through GSM. %s, Serial:%s, IP:%s\r\n\r\n", FW_VER, OTADRIVE.getChipId().c_str(), gprs_ip.c_str());
}

void update_prgs(size_t downloaded, size_t total)
{
  // LCD Resolution: 135 x 240
  int percent = downloaded / (total / 100 + 1);
  Serial.printf("upgrade %d/%d  %d%%\r\n\r\n", downloaded, total, percent);
  
  tft.setTextSize(2);
  tft.setCursor(0, 80);
  tft.printf("%d/%d   %d%%", downloaded, total, percent);
  tft.fillRect(0, 100, percent * 2.4, 10, TFT_GREEN);
  tft.drawRect(0, 100, 240, 10, TFT_GREEN);
}

void draw_lcd(uint8_t step, const char *txt)
{
  // tft.fillScreen(TFT_BLACK);
  // Resolution: 135 x 240
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.print("App ");
  tft.print(FW_VER);
  tft.drawLine(0, 32, 256, 32, TFT_BLUE);
  // show Lamps
  tft.fillRect(0, 125 - 5, 240, 11, TFT_BLACK);
  tft.fillCircle(5 + (10 * (counter % 24)), 125, 5, TFT_YELLOW);

  // show message
  if (step == 0)
  {
    tft.setCursor(0, 44);
    tft.print(txt);
  }
  else if (step == 1)
  {
    tft.setCursor(0, 44);
    tft.printf("%s %5d", txt, ++counter);
  }

  tft.setTextSize(1);
  tft.setCursor(0, 20);
  tft.printf("IP: %s", gprs_ip.c_str());
}

void start_lcd()
{
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  draw_lcd(0, "WELCOME");
}

void setup()
{
  // Important Notice: Please enable log outputs. Tools->Core Debug Level->Info
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, 21, 22);

  printInfo();
  start_lcd();

  // Setup WiFi
  // We dont need Wi-Fi here

  SPIFFS.begin(true);
  OTADRIVE.setInfo(APIKEY, FW_VER);
  OTADRIVE.onUpdateFirmwareProgress(update_prgs);

  // do something about power-on GSM
}

bool modem_ready = false;
void loop()
{
  for (uint8_t tr = 0; tr < 5; tr++)
  {
    draw_lcd(1, modem_ready ? "CNT" : "GSM");
    printInfo();
    delay(1000);
    if (!modem.testAT(100))
    {
      Serial.println("Testing modem RX/TX test faild\r\n");
      continue;
    }

    if (modem.getSimStatus(100) != SimStatus::SIM_READY)
    {
      Serial.println("Testing modem SIMCARD faild\r\n");
      continue;
    }
    if (!modem.isGprsConnected())
    {
      Serial.println("Testing modem internet faild. Try to connect ...\r\n");
      modem.gprsConnect("irancell");
      continue;
    }

    gprs_ip = modem.getLocalIP();
    modem_ready = true;
    Serial.println("Modem is ready\r\n");
  }

  if (OTADRIVE.timeTick(30))
  {
    Serial.println("Lets updaete the firmware\r\n");
    if (modem.isGprsConnected() || modem.isGprsConnected())
    {
      // auto c = OTADRIVE.getConfigs(gsm_otadrive_client);
      // Serial.printf("config %s\n", c.c_str());

      draw_lcd(1, "Checking");
      auto inf = OTADRIVE.updateFirmwareInfo(gsm_otadrive_client);
      if (inf.available)
      {
        draw_lcd(1, "UPDATING");
        OTADRIVE.updateFirmware(gsm_otadrive_client);
      }
    }
    else
    {
      Serial.println("Modem is not ready\r\n");
    }
  }
  else
  {
    Serial.println("Waiting\r\n");
  }
}
