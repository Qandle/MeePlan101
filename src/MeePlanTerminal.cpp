//Modifed from HTTPS GET Example
//to call a REST api and displaying data on WIO Terminal's screen.

#include <ArduinoJson.h>
#include <rpcWiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "TFT_eSPI.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

TFT_eSPI tft;
const char *ssid = "wifi here";
const char *password = "password here";
const char *test_root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFgTCCBGmgAwIBAgIQOXJEOvkit1HX02wQ3TE1lTANBgkqhkiG9w0BAQwFADB7\n"
    "MQswCQYDVQQGEwJHQjEbMBkGA1UECAwSR3JlYXRlciBNYW5jaGVzdGVyMRAwDgYD\n"
    "VQQHDAdTYWxmb3JkMRowGAYDVQQKDBFDb21vZG8gQ0EgTGltaXRlZDEhMB8GA1UE\n"
    "AwwYQUFBIENlcnRpZmljYXRlIFNlcnZpY2VzMB4XDTE5MDMxMjAwMDAwMFoXDTI4\n"
    "MTIzMTIzNTk1OVowgYgxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpOZXcgSmVyc2V5\n"
    "MRQwEgYDVQQHEwtKZXJzZXkgQ2l0eTEeMBwGA1UEChMVVGhlIFVTRVJUUlVTVCBO\n"
    "ZXR3b3JrMS4wLAYDVQQDEyVVU0VSVHJ1c3QgUlNBIENlcnRpZmljYXRpb24gQXV0\n"
    "aG9yaXR5MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAgBJlFzYOw9sI\n"
    "s9CsVw127c0n00ytUINh4qogTQktZAnczomfzD2p7PbPwdzx07HWezcoEStH2jnG\n"
    "vDoZtF+mvX2do2NCtnbyqTsrkfjib9DsFiCQCT7i6HTJGLSR1GJk23+jBvGIGGqQ\n"
    "Ijy8/hPwhxR79uQfjtTkUcYRZ0YIUcuGFFQ/vDP+fmyc/xadGL1RjjWmp2bIcmfb\n"
    "IWax1Jt4A8BQOujM8Ny8nkz+rwWWNR9XWrf/zvk9tyy29lTdyOcSOk2uTIq3XJq0\n"
    "tyA9yn8iNK5+O2hmAUTnAU5GU5szYPeUvlM3kHND8zLDU+/bqv50TmnHa4xgk97E\n"
    "xwzf4TKuzJM7UXiVZ4vuPVb+DNBpDxsP8yUmazNt925H+nND5X4OpWaxKXwyhGNV\n"
    "icQNwZNUMBkTrNN9N6frXTpsNVzbQdcS2qlJC9/YgIoJk2KOtWbPJYjNhLixP6Q5\n"
    "D9kCnusSTJV882sFqV4Wg8y4Z+LoE53MW4LTTLPtW//e5XOsIzstAL81VXQJSdhJ\n"
    "WBp/kjbmUZIO8yZ9HE0XvMnsQybQv0FfQKlERPSZ51eHnlAfV1SoPv10Yy+xUGUJ\n"
    "5lhCLkMaTLTwJUdZ+gQek9QmRkpQgbLevni3/GcV4clXhB4PY9bpYrrWX1Uu6lzG\n"
    "KAgEJTm4Diup8kyXHAc/DVL17e8vgg8CAwEAAaOB8jCB7zAfBgNVHSMEGDAWgBSg\n"
    "EQojPpbxB+zirynvgqV/0DCktDAdBgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rID\n"
    "ZsswDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQFMAMBAf8wEQYDVR0gBAowCDAG\n"
    "BgRVHSAAMEMGA1UdHwQ8MDowOKA2oDSGMmh0dHA6Ly9jcmwuY29tb2RvY2EuY29t\n"
    "L0FBQUNlcnRpZmljYXRlU2VydmljZXMuY3JsMDQGCCsGAQUFBwEBBCgwJjAkBggr\n"
    "BgEFBQcwAYYYaHR0cDovL29jc3AuY29tb2RvY2EuY29tMA0GCSqGSIb3DQEBDAUA\n"
    "A4IBAQAYh1HcdCE9nIrgJ7cz0C7M7PDmy14R3iJvm3WOnnL+5Nb+qh+cli3vA0p+\n"
    "rvSNb3I8QzvAP+u431yqqcau8vzY7qN7Q/aGNnwU4M309z/+3ri0ivCRlv79Q2R+\n"
    "/czSAaF9ffgZGclCKxO/WIu6pKJmBHaIkU4MiRTOok3JMrO66BQavHHxW/BBC5gA\n"
    "CiIDEOUMsfnNkjcZ7Tvx5Dq2+UUTJnWvu6rvP3t3O9LEApE9GQDTF1w52z97GA1F\n"
    "zZOFli9d31kWTz9RvdVFGD/tSo7oBmF0Ixa1DVBzJ0RHfxBdiSprhTEUxOipakyA\n"
    "vGp4z7h/jnZymQyd/teRCBaho1+V\n"
    "-----END CERTIFICATE-----\n";



void drawTab()
{
  tft.drawRect(0, 0, SCREEN_WIDTH, 30, TFT_LIGHTGREY);
  tft.fillRect(0, 0, SCREEN_WIDTH, 30, TFT_LIGHTGREY);
}

void drawArrows(){
  tft.drawTriangle(270, 80, 290, 40, 310, 80, TFT_DARKGREY);
  tft.fillTriangle(270, 80, 290, 40, 310, 80, TFT_DARKGREY);
  tft.drawTriangle(270, 190, 290, 230, 310, 190, TFT_DARKGREY);
  tft.fillTriangle(270, 190, 290, 230, 310, 190, TFT_DARKGREY);
}

void drawCursor(){

}

void drawMenu(int32_t x, int32_t y){
  tft.drawRoundRect(x, y, 265, 45, 5, TFT_DARKGREY);
  tft.fillRoundRect(x, y, 265, 45, 5, TFT_DARKGREY);
}

WiFiClientSecure client;
DynamicJsonDocument doc(4096);

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  //screen setup
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE); // fills entire the screen with colour red
  tft.setTextSize(3);
  tft.setTextColor(TFT_BLACK);
  tft.setTextWrap(true);
  drawTab();
  drawArrows();
  tft.drawString("Connecting...", 0, 0);
  drawMenu(0,0);
  while (WiFi.status() != WL_CONNECTED)
  { //Check for the connection
    delay(500);
    Serial.println("Connecting..");
  }
  
  Serial.print("Connected to the WiFi network with IP: ");
  Serial.println(WiFi.localIP());
  client.setCACert(test_root_ca);
}

void loop()
{
  if (&client)
  {
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;
      Serial.print("[HTTPS] begin...\n");
      //google ip as an example lol;
      if (https.begin(client, "https://www.timeapi.io/api/Time/current/zone?timeZone=Asia/Bangkok"))
      { // HTTPS
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
          // file found at server

          String payload = https.getString();
          deserializeJson(doc, payload);
          JsonObject arr = doc.as<JsonObject>();
          String text = arr["time"];
          drawTab();
          tft.setTextDatum(TR_DATUM);
          tft.drawString(text, 315, 5,1);
          Serial.println(text);
          Serial.println(payload);
        }
        else
        {
          Serial.printf("[HTTPS] GET... failed, error: %s, code: %d\n", https.errorToString(httpCode).c_str(), httpCode);
        }
        https.end();
      }
      else
      {
        Serial.printf("[HTTPS] Unable to connect\n");
      }
      // End extra scoping block
    }
  }
  else
  {
    Serial.println("Unable to create client");
  }
  Serial.println();
  Serial.println("Waiting 10s before the next round...");
  delay(5000);
}
