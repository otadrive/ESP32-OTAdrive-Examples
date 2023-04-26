#include <Arduino.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// configuration variables
int onDelay;
int offDelay;

// To inject firmware info into binary file, You have to use following macro according to let
// OTAdrive to detect binary info automatically
#define ProductKey "c0af643b-4f90-4905-9807-db8be5164cde" // Replace with your own APIkey
#define Version "v@1.0.0.7"
#define MakeFirmwareInfo(k, v) "&_FirmwareInfo&k=" k "&v=" v "&FirmwareInfo_&"

// OTAdrive.com CA certificate
const char otadrv_ca[] =
    "-----BEGIN CERTIFICATE-----\n\
MIIEzjCCA7agAwIBAgIQJt3SK0bJxE1aaU05gH5yrTANBgkqhkiG9w0BAQsFADB+\n\
MQswCQYDVQQGEwJQTDEiMCAGA1UEChMZVW5pemV0byBUZWNobm9sb2dpZXMgUy5B\n\
LjEnMCUGA1UECxMeQ2VydHVtIENlcnRpZmljYXRpb24gQXV0aG9yaXR5MSIwIAYD\n\
VQQDExlDZXJ0dW0gVHJ1c3RlZCBOZXR3b3JrIENBMB4XDTE0MDkxMTEyMDAwMFoX\n\
DTI3MDYwOTEwNDYzOVowgYUxCzAJBgNVBAYTAlBMMSIwIAYDVQQKExlVbml6ZXRv\n\
IFRlY2hub2xvZ2llcyBTLkEuMScwJQYDVQQLEx5DZXJ0dW0gQ2VydGlmaWNhdGlv\n\
biBBdXRob3JpdHkxKTAnBgNVBAMTIENlcnR1bSBEb21haW4gVmFsaWRhdGlvbiBD\n\
QSBTSEEyMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoSVj343kIAfZ\n\
VNHRBPYX4j5H+8N0JbjEvxISvOBw0TkFwhez94JwoE4H/hAq/9sNRl4klKOLRZ8Y\n\
m85CxK7bgzO8wru0MLanN4d4e0jLJSyCuwpIEmB2ieyOzI8eUkjphgJawrCKfIU9\n\
2f9gTzNspqGgheHXU/LqJz1lqXLBCIPMsCWcEUYk4D70p+/tUbFlk0K09uaGChB5\n\
MjZYsmuo3NV6Hp0U7kDnskZMvZopwuz4MMFiAiriHINi0IU2GoPeEoQpZe/SMr4x\n\
YEKoz/jd6tBWRx29dpYkE+e+2Zkr+jBk8Yo4eqbhKpYCsJ262I9tTnqUaX2wk6p0\n\
5ZOQE/qimQIDAQABo4IBPjCCATowDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQU\n\
5TGtvzoRlvSDvFA81LeQm5Du3iUwHwYDVR0jBBgwFoAUCHbNywf/JPbFze27kLzi\n\
hDdGdfcwDgYDVR0PAQH/BAQDAgEGMC8GA1UdHwQoMCYwJKAioCCGHmh0dHA6Ly9j\n\
cmwuY2VydHVtLnBsL2N0bmNhLmNybDBrBggrBgEFBQcBAQRfMF0wKAYIKwYBBQUH\n\
MAGGHGh0dHA6Ly9zdWJjYS5vY3NwLWNlcnR1bS5jb20wMQYIKwYBBQUHMAKGJWh0\n\
dHA6Ly9yZXBvc2l0b3J5LmNlcnR1bS5wbC9jdG5jYS5jZXIwOQYDVR0gBDIwMDAu\n\
BgRVHSAAMCYwJAYIKwYBBQUHAgEWGGh0dHA6Ly93d3cuY2VydHVtLnBsL0NQUzAN\n\
BgkqhkiG9w0BAQsFAAOCAQEAur/w4d1NK0JDZFjfZPP/gBpfVr47qbJ291R6TDDB\n\
mSRLctLK1PoIxpDeiBLt+JD5/KmE/ZLyeOXbySJXq0EwQmsLn9dzM/sBZxxCXI8n\n\
Z8duBwONDpbLCgPMPviHPDUwzRiM1XHdzd1hsBOjZEZO/nFOa2XpFATyP6i9DDY9\n\
Kl2eB/LCT5DFXk0YN9EnKICkNuXKk2plDviTua9SWEt6cdi68+/S8/ail+RdFAKa\n\
y+WutpPhI5+bP0b37o6hAFtmwx5oI4YPXXe6U635UvtwFcV16895rUl88nZirkQv\n\
xV9RNCVBahIKX46uEMRDiTX97P8x5uweh+k6fClQRUGjFA==\n\
-----END CERTIFICATE-----";

void update();
bool updateConfigs();
void saveConfigs();
void loadConfigs();

void setup()
{
  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  WiFi.begin("smarthomehub", "****");

  EEPROM.begin(32);
  loadConfigs();

  Serial.begin(115200);
}

uint32_t updateCounter = 0;

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(2, 1);
  delay(onDelay);
  digitalWrite(2, 0);
  delay(offDelay);
  Serial.printf("blink: %d\n", updateCounter);

  if (WiFi.status() == WL_CONNECTED)
  {
    updateCounter++;
    if (updateCounter > 5)
    {
      updateCounter = 0;
      update();
      updateConfigs();
    }
  }
}

String getChipId()
{
  String ChipIdHex = String((uint32_t)(ESP.getEfuseMac() >> 32), HEX);
  ChipIdHex += String((uint32_t)ESP.getEfuseMac(), HEX);
  return ChipIdHex;
}

void saveConfigs()
{
  EEPROM.writeInt(4, onDelay);
  EEPROM.writeInt(8, offDelay);
  EEPROM.commit();
}

void loadConfigs()
{
  // check configs initialized in eeprom or not
  if (EEPROM.readInt(0) != 0x4A)
  {
    // configs not initialized yet. write for first time
    EEPROM.writeInt(0, 0x4A); // memory sign
    EEPROM.writeInt(4, 200);  // onDelay
    EEPROM.writeInt(8, 250);  // offDelay
    EEPROM.commit();
  }
  else
  {
    // configs initialized and valid. read values
    onDelay = EEPROM.readInt(4);
    offDelay = EEPROM.readInt(8);
  }
}

bool updateConfigs()
{
  String url = "https://www.otadrive.com/deviceapi/getconfig?";
  url += MakeFirmwareInfo(ProductKey, Version);
  url += "&s=" + getChipId();

  WiFiClientSecure client;
  HTTPClient http;

  // set SSL/TLS certificate
  client.setCACert(otadrv_ca);

  // set desired timeouts
  client.setTimeout(1);
  http.setConnectTimeout(1000);
  http.setTimeout(1000);

  // make secure connection to server
  Serial.println(url);
  if (!client.connect("otadrive.com", 443))
    return false;
  if (!http.begin(client, url))
    return false;

  // Send GET Request
  int httpCode = http.GET();
  Serial.printf("http code: %d\n", httpCode);

  // httpCode will be negative on error
  if (httpCode != HTTP_CODE_OK)
    return false;

  // extract and deserialize response
  String payload = http.getString();
  DynamicJsonDocument doc(512);
  deserializeJson(doc, payload);
  Serial.printf("http content: %s\n", payload.c_str());

  // check response has all necessary fields. otherwise MCU may crash of fetch
  if (!doc.containsKey("onDelay") ||
      !doc.containsKey("offDelay"))
    return false;

  onDelay = doc["onDelay"].as<int>();
  offDelay = doc["offDelay"].as<int>();
  saveConfigs();
  return true;
}

void update()
{
  String url = "http://otadrive.com/deviceapi/update?";
  url += MakeFirmwareInfo(ProductKey, Version);
  url += "&s=" + getChipId();

  httpUpdate.setLedPin(2);
  WiFiClient client;
  httpUpdate.update(client, url, Version);
}
