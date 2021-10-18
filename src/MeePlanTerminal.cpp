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

void drawMenu(int32_t x, int32_t y, uint32_t color){
  tft.drawRoundRect(x, y, 260, 45, 5, color);
  tft.fillRoundRect(x, y, 260, 45, 5, color);
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
  drawMenu(5,35,TFT_DARKGREY);
  drawMenu(5,85,TFT_DARKGREY);
  drawMenu(5,135,TFT_DARKGREY);
  drawMenu(5,185,TFT_DARKGREY);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("00:00", 315, 5);
}

void loop()
{
  delay(5000);
}
