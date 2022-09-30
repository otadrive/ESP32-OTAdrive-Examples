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
const char *HOME_HTML = "<html><body style='width:250px'>"
                        "<div style='background-color:GREEN'> <a href='/?cmd=on_g'>LED GREEN ON</a> </div>"
                        "<div style='background-color:DARKGREEN'> <a href='/?cmd=off_g'>LED GREEN OFF</a> </div>"
                        "<br/>"
                        "<div style='background-color:RED'> <a href='/?cmd=on_r'>LED RED ON</a> </div>"
                        "<div style='background-color:DARKRED'> <a href='/?cmd=off_r'>LED RED OFF</a> </div>"
                        "</body></html>";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

bool process_cmd(AsyncWebServerRequest *request)
{
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
  if (process_cmd(request))
    request->send(200, "text/html", HOME_HTML);
  else
    request->send(200, "text/html", HOME_HTML);
}

void server_post_home(AsyncWebServerRequest *request)
{
  process_cmd(request);
  request->send(200, "text/plain", HOME_HTML);
}

void setup_server()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, server_get_home);

  server.on("/", HTTP_POST, server_post_home);

  server.onNotFound(notFound);

  server.begin();
}

void setup()
{
  Serial.begin(115200);

  // setup LED's
  for (int i = 0; i < 5; i++)
    pinMode(leds[i], OUTPUT);

  setup_server();
}

void loop()
{
}
