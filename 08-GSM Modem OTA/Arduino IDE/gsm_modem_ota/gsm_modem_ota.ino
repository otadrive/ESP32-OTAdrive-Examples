// Important Notice: Please enable log outputs. Tools->Core Debug Level->Debug

#include <otadrive_esp.h>
#include <Arduino.h>

// install TinyGSM library (v0.11.7 or newer)
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@8.21.3"          // this app version
#define LED 2
#define WIFI_SSID "OTAdrive"
#define WIFI_PASS "@tadr!ve"

TinyGsm modem(Serial);
// MuxChannel 0 for MQTT
TinyGsmClient gsm_mqtt_client(modem, 0);
// MuxChannel 1 for OTA
TinyGsmClient gsm_otadrive_client(modem, 1);

void printInfo()
{
  log_i("Example08: OTA through GSM. %s, Serial:%s, IP:%s", FW_VER, OTADRIVE.getChipId().c_str(), WiFi.localIP().toString().c_str());
}

void update_prgs(size_t i, size_t total)
{
  Serial.printf("upgrade %d/%d   %d%%\n", i, total, ((i * 100) / total));
}

void setup()
{
  Serial.begin(115200);

  // Setup WiFi
  // We dont need Wi-Fi here

  SPIFFS.begin(true);
  OTADRIVE.setInfo(APIKEY, FW_VER);
  OTADRIVE.onUpdateFirmwareProgress(update_prgs);

  printInfo();

  // do something about power-on GSM
}

void loop()
{
  printInfo();

  for (uint8_t tr = 0; tr < 5; tr++)
  {
    delay(3000);
    if (!modem.testAT(100))
    {
      Serial.println("Testing modem RX/TX test faild");
      continue;
    }

    if (modem.getSimStatus(100) != SimStatus::SIM_READY)
    {
      Serial.println("Testing modem SIMCARD faild");
      continue;
    }
    if (!modem.isGprsConnected())
    {
      Serial.println("Testing modem internet faild. Try to connect ...");
      modem.gprsConnect("simbase", "", "");
      continue;
    }

    Serial.println("Modem is ready");
  }

  if (OTADRIVE.timeTick(30))
  {
    Serial.println("Lets updaete the firmware");
    if (modem.isGprsConnected())
    {
      // auto a = OTADRIVE.updateFirmwareInfo(gsm_otadrive_client);
      // Serial.printf("info: %d, %d, %s\n", a.available, a.size, a.version.c_str());
      // auto c = OTADRIVE.getConfigs(gsm_otadrive_client);
      // Serial.printf("config %s\n", c.c_str());
      // OTADRIVE.sendAlive(gsm_otadrive_client);
      Serial.printf("isGprsConnected");
      OTADRIVE.updateFirmware(gsm_otadrive_client);
    }
    else
    {
      Serial.println("Modem is not ready");
    }
  }
  else
  {
    Serial.println("Waiting");
  }
}
