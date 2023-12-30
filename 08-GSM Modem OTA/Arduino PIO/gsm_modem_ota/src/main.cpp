#include <otadrive_esp.h>
#include <Arduino.h>
// install TinyGSM library (v0.11.7 or newer)
#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>

#define APIKEY "5ec34eab-c516-496d-8cb0-78dc4744af3b" // OTAdrive APIkey for this product
#define FW_VER "v@8.21.3"                             // this app version

TinyGsm modem(Serial);
// MuxChannel 0 for MQTT
TinyGsmClient gsm_mqtt_client(modem, 0);
// MuxChannel 1 for OTA
TinyGsmClient gsm_otadrive_client(modem, 1);

String gprs_ip;

void printInfo()
{
  Serial.printf("Example08: OTA through GSM. %s, Serial:%s, IP:%s\r\n\r\n", FW_VER, OTADRIVE.getChipId().c_str(), gprs_ip.c_str());
}

void update_prgs(size_t i, size_t total)
{
  Serial.printf("upgrade %d/%d   %d%%\r\n\r\n", i, total, ((i * 100) / total));
}

void setup()
{
  // Important Notice: Please enable log outputs. Tools->Core Debug Level->Info
  Serial.begin(115200);
  printInfo();

  // Setup WiFi
  // We dont need Wi-Fi here

  SPIFFS.begin(true);
  OTADRIVE.setInfo(APIKEY, FW_VER);
  OTADRIVE.onUpdateFirmwareProgress(update_prgs);

  // do something about power-on GSM
}

void loop()
{
  for (uint8_t tr = 0; tr < 5; tr++)
  {
    printInfo();
    delay(3000);
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
      modem.gprsConnect("simbase", "", "");
      continue;
    }

    gprs_ip = modem.getLocalIP();
    Serial.println("Modem is ready\r\n");
  }

  if (OTADRIVE.timeTick(30))
  {
    Serial.println("Lets updaete the firmware\r\n");
    if (modem.isGprsConnected() || modem.isGprsConnected())
    {
      // auto a = OTADRIVE.updateFirmwareInfo(gsm_otadrive_client);
      // Serial.printf("info: %d, %d, %s\n", a.available, a.size, a.version.c_str());
      // auto c = OTADRIVE.getConfigs(gsm_otadrive_client);
      // Serial.printf("config %s\n", c.c_str());
      // OTADRIVE.sendAlive(gsm_otadrive_client);
      Serial.println("isGprsConnected\r\n");
      OTADRIVE.updateFirmware(gsm_otadrive_client);
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
