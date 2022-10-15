#include <otadrive_esp.h>
#include <Arduino.h>
#include <gsmHTTPUpdate.h>
#include <TinyGsmClient.h>

#define LED_W 4
#define LED_B 16
#define LED_G 17
#define LED_Y 18
#define LED_R 19
#define BTN1 13
#define BTN2 12
const int leds[] = {LED_W, LED_B, LED_G, LED_Y, LED_R};

const char *ssid = "OTAdrive";
const char *password = "@tadr!ve";
TinyGsm modem(Serial);
// MuxChannel 0 for MQTT
TinyGsmClient gsm_mqtt_client(modem, 0);
// MuxChannel 1 for OTA
TinyGsmClient gsm_otadrive_client(modem, 1);

void setup()
{
  Serial.begin(115200);
  Serial.println("Download a new firmware from SIM800");

  // setup LED's
  for (int i = 0; i < 5; i++)
    pinMode(leds[i], OUTPUT);

  // Setup WiFi
  // We dont need Wi-Fi here

  SPIFFS.begin(true);
  OTADRIVE.setInfo("YOUR_PRODUCT_APIKEY", "YOUR_FIRMWARE_VERSION");

  // do something about power-on GSM
}

void loop()
{
  if (OTADRIVE.timeTick(60))
  {
  }

  if (modem.testAT(100))
  {
    if (modem.getSimStatus(100) == SimStatus::SIM_READY)
    {
      if (!modem.isGprsConnected())
      {
        // set your APN info here
        modem.gprsConnect("mcinet", "", "");
      }

      if (modem.isGprsConnected())
      {
        OTAdrive::GsmHTTPUpdate gsmHttpUpdate;
        gsmHttpUpdate.update(gsm_otadrive_client, "otadrive.com/deviceapi/update?k=bd076abe-a423-4880-85b3-4367d07c8eda&s=11&v=1.0.0");
      }
    }
  }

  delay(3000);
}
