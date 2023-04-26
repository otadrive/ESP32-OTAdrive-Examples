#include <otadrive_esp.h>
#include <Arduino.h>
#include <TinyGsmClient.h>

TinyGsm modem(Serial);
// MuxChannel 0 for MQTT
TinyGsmClient gsm_mqtt_client(modem, 0);
// MuxChannel 1 for OTA
TinyGsmClient gsm_otadrive_client(modem, 1);

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
  OTADRIVE.setInfo("YOUR_APIKEY", "v@1.1.1");
  OTADRIVE.onUpdateFirmwareProgress(update_prgs);

  Serial.printf("Download a new firmware from SIM800, V=%s\n", OTADRIVE.Version.c_str());

  // do something about power-on GSM
}

void loop()
{
  if (!OTADRIVE.timeTick(30))
  {
    delay(3000);
    return;
  }

  for (uint8_t tr = 0; tr < 3; tr++)
  {
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
          // auto a = OTADRIVE.updateFirmwareInfo(gsm_otadrive_client);
          // Serial.printf("info: %d, %d, %s\n", a.available, a.size, a.version.c_str());
          // auto c = OTADRIVE.getConfigs(gsm_otadrive_client);
          // Serial.printf("config %s\n", c.c_str());
          // OTADRIVE.sendAlive(gsm_otadrive_client);

          OTADRIVE.updateFirmware(gsm_otadrive_client);
        }
      }
    }
  }
}
