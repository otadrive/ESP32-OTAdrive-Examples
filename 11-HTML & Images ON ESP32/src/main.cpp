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
const char *PARAM_CMD = "cmd";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

bool process_cmd(AsyncWebServerRequest *request)
{
  Serial.printf("%s\n", request->url().c_str());
  String command;
  if (request->hasArg(PARAM_CMD))
  {
    command = request->arg(PARAM_CMD);

    if (command.equals("on_r"))
      digitalWrite(LED_R, HIGH);
    else if (command.equals("off_r"))
      digitalWrite(LED_R, LOW);
    else if (command.equals("on_g"))
      digitalWrite(LED_G, HIGH);
    else if (command.equals("off_g"))
      digitalWrite(LED_G, LOW);
    else
      return false;
    return true;
  }

  return false;
}

void server_get_home(AsyncWebServerRequest *request)
{
  process_cmd(request);
  request->send(SPIFFS, "/static_html/index.html", "text/html");
}

void setup_server()
{
#ifdef ESP32
  server.addHandler(new SPIFFSEditor(SPIFFS));
#elif defined(ESP8266)
  server.addHandler(new SPIFFSEditor());
#endif

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
  OTADRIVE.setInfo("bd076abe-a423-4880-85b3-4367d07c8eda", "1.0.0");
}

void loop()
{
  if (OTADRIVE.timeTick(60))
    OTADRIVE.syncResources();
}
