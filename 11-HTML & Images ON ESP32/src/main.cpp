#include <otadrive_esp.h>

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

#define LED_W 4
#define LED_B 16
#define LED_G 17
#define LED_Y 18
#define LED_R 19
#define BTN1 13
#define BTN2 12
const int leds[] = {LED_W, LED_B, LED_G, LED_Y, LED_R};

AsyncWebServer server(80);

const char *ssid = "OTAdrive";
const char *password = "@tadr!ve";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void server_get_home(AsyncWebServerRequest *request)
{
  Serial.printf("%s\n", request->url().c_str());
  if (request->hasArg("cmd"))
  {
    String command = request->arg("cmd");

    if (command.equals("on_r"))
      digitalWrite(LED_R, HIGH);
    else if (command.equals("off_r"))
      digitalWrite(LED_R, LOW);
    else if (command.equals("on_g"))
      digitalWrite(LED_G, HIGH);
    else if (command.equals("off_g"))
      digitalWrite(LED_G, LOW);
  }

  request->send(SPIFFS, "/static_html/index.html", "text/html");
}

void setup_server()
{
  server.addHandler(new SPIFFSEditor(SPIFFS));
  server.on("/", HTTP_GET, server_get_home);
  server.serveStatic("/", SPIFFS, "/static_html/").setDefaultFile("index.html");
  server.onNotFound(notFound);

  server.begin();
}

void setup()
{
  Serial.begin(115200);

  // setup LED's
  for (int i = 0; i < 5; i++)
    pinMode(leds[i], OUTPUT);

  // Setup WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  setup_server();

  SPIFFS.begin(true);
  OTADRIVE.setInfo("YOUR_PRODUCT_APIKEY", "YOUR_FIRMWARE_VERSION");
}

void loop()
{
  if (OTADRIVE.timeTick(60))
    OTADRIVE.syncResources();

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  delay(3000);
}
