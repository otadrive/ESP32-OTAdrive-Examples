#include <LiquidCrystal.h>
#include <otadrive_esp.h>

// LCD pin to ESP32
const int pin_RS = 12;
const int pin_EN = 13;
const int pin_d4 = 17;
const int pin_d5 = 16;
const int pin_d6 = 27;
const int pin_d7 = 14;
const int pin_BL = -1;

LiquidCrystal lcd(pin_RS, pin_EN, pin_d4, pin_d5, pin_d6, pin_d7);
void OnProgress(int progress, int totalt);

void setup()
{
  OTADRIVE.setInfo("888ac717-12f0-4da2-807a-c55bb70d1386", "1.0.4");
  OTADRIVE.onUpdateFirmwareProgress(OnProgress);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("OTAdrive");
  lcd.setCursor(0, 1);
  lcd.print("App. V1.0.4");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("App. V1.0.4");

  WiFi.begin("OTAdrive", "@tadr!ve");
}

int dummyCounter = 0;

void loop()
{
  lcd.setCursor(0, 1);
  lcd.printf("CNT %d", dummyCounter);
  dummyCounter++;

  if (WiFi.status() == WL_CONNECTED)
  {
    lcd.setCursor(7, 1);
    lcd.print("WIFI");

    if (OTADRIVE.timeTick(20))
    {
      auto inf = OTADRIVE.updateFirmwareInfo();
      if (inf.available)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.printf("Upgrading %s", inf.version.c_str());
        OTADRIVE.updateFirmware();
      }
    }
  }
  delay(500);
}

int last = 0;
void OnProgress(int progress, int totalt)
{
  int progressPercent = (100 * progress) / totalt;
  Serial.print(".");
  if (last != progressPercent && progressPercent % 10 == 0)
  {
    // print every 10%
    lcd.setCursor(0, 1);
    for (uint8_t i = 0; i < progressPercent; i += 6)
      lcd.print("=");
    lcd.setCursor(6, 1);
    lcd.printf("%d%%", progressPercent);
    Serial.println(progressPercent);
  }
  last = progressPercent;
}