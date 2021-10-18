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
  tft.drawRoundRect(x, y, 260, 45, 5, TFT_DARKGREY);
  tft.fillRoundRect(x, y, 260, 45, 5, TFT_DARKGREY);
}


void setup()
{
  Serial.begin(115200);
  //screen setup
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE); // fills entire the screen with colour red
  tft.setTextSize(3);
  tft.setTextColor(TFT_BLACK);
  tft.setTextWrap(true);
  drawTab();
  drawArrows();
  drawMenu(5,35);
  drawMenu(5,85);
  drawMenu(5,135);
  drawMenu(5,185);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("00:00", 315, 5,1);
}

void loop()
{
  delay(5000);
}
