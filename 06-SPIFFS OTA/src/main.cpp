#include <Arduino.h>
#include <otadrive_esp.h>

void update();
void setup()
{
  Serial.begin(115200);

  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("smarthomehub", "smarthome2015");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(400);
  }

  Serial.println(WiFi.localIP());

// initialize FileSystem
#ifdef ESP8266
  if (!fileObj.begin())
  {
    Serial.println("FFS Mount Failed");
    fileObj.format();
    return;
  }
#elif defined(ESP32)
  if (!fileObj.begin(true))
  {
    Serial.println("FFS Mount Failed");
    return;
  }
#endif

  //
  OTADRIVE.setInfo("1c70ece5-07b2-4ee3-9ee3-9b9f566dfb2e", "v@2.5.5");
}

uint32_t updateCounter = 0;

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
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(1000);
  if (WiFi.status() == WL_CONNECTED)
  {
    // every 10 seconds
    if (OTADRIVE.timeTick(10))
    {
      digitalWrite(2, HIGH);
      delay(100);
      digitalWrite(2, LOW);
      delay(400);
      Serial.println("\n\nlets get data");
      updateCounter = 0;

      updateInfo inf = OTADRIVE.updateFirmwareInfo();
      Serial.printf("\nfirmware info: %s ,%dBytes\n%s\n",
                    inf.version.c_str(), inf.size, inf.available ? "New version available" : "You are already update");
      OTADRIVE.syncResources();
      listDir(fileObj, "/", 0);
      OTADRIVE.updateFirmware();
    }
  }
}
