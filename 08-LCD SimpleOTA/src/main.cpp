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

void setup()
{
  OTADRIVE.setInfo("888ac717-12f0-4da2-807a-c55bb70d1386", "1.0.3");

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("OTAdrive");
  lcd.setCursor(0, 1);
  lcd.print("App. V1.0.3");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("App. V1.0.3");

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

    if (OTADRIVE.timeTick(10))
    {
      auto r = OTADRIVE.updateFirmware();
      lcd.print((int)r.code);
    }
  }
  delay(500);
}
