//Modifed from HTTPS GET Example
//to call a REST api and displaying data on WIO Terminal's screen.

#include <ArduinoJson.h>
#include <rpcWiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "TFT_eSPI.h"
#include "Free_Fonts.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

TFT_eSPI tft;
const char *ssid = "wifi here";
const char *password = "password here";
int check_menu_logo = 0;

int text_height = 0;
int text_width = 0;

void drawTab()
{
  tft.drawRect(0, 0, SCREEN_WIDTH, 30, TFT_LIGHTGREY);
  tft.fillRect(0, 0, SCREEN_WIDTH, 30, TFT_LIGHTGREY);
}

void drawArrows()
{
  tft.drawTriangle(270, 80, 290, 40, 310, 80, TFT_DARKGREY);
  tft.fillTriangle(270, 80, 290, 40, 310, 80, TFT_DARKGREY);
  tft.drawTriangle(270, 190, 290, 230, 310, 190, TFT_DARKGREY);
  tft.fillTriangle(270, 190, 290, 230, 310, 190, TFT_DARKGREY);
}

void drawCursor()
{
}

void drawSelectbox(int32_t x, int32_t y, uint32_t color)
{
  tft.drawRoundRect(x, y, 260, 45, 5, color);
  tft.fillRoundRect(x, y, 260, 45, 5, color);
}

void MeePlan_Logo()
{

  tft.fillScreen(TFT_PURPLE);
  tft.setFreeFont(&FreeSansBoldOblique24pt7b);
  tft.setTextColor(TFT_LIGHTGREY);
  tft.setTextSize(1);
  tft.drawString("Mee Plan", 60, 100);

  while (check_menu_logo != 1)
  {
    tft.setFreeFont(&FreeSansBoldOblique12pt7b);
    tft.drawString("click to continue..", 60, 180);
    text_height = tft.fontHeight();
    text_width = tft.textWidth("click to continue..");
    tft.fillRect(60, 180, text_width, text_height, TFT_PURPLE);
    delay(800);
    tft.drawString("click to continue..", 60, 180);
    delay(800);
    if (digitalRead(WIO_KEY_A) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_KEY_B) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_KEY_C) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_5S_UP) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_5S_DOWN) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_KEY_LEFT) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_KEY_RIGHT) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_KEY_PRESS) == LOW)
    {
      check_menu_logo = 1;
    }
  }
}

void drawMenu()
{
}

void drawSetting()
{
}

void setup()
{
  tft.begin();
  tft.setRotation(3);
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);
  //screen setup
  /*
  tft.fillScreen(TFT_WHITE); // fills entire the screen with colour red
  tft.setTextSize(3);
  tft.setTextColor(TFT_BLACK);
  tft.setTextWrap(true);
  drawTab();
  drawArrows();
  drawSelectbox(5,35,TFT_DARKGREY);
  drawSelectbox(5,85,TFT_DARKGREY);
  drawSelectbox(5,135,TFT_DARKGREY);
  drawSelectbox(5,185,TFT_DARKGREY);
  tft.setTextDatum(TR_DATUM);
  tft.drawString("00:00", 315, 5);
  */
  MeePlan_Logo();
}

void loop()
{

  delay(5000);
}
