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

#define MEE_GREYPURPLE 0x526B
#define MEE_LIGHTPURPLE 0xE71F

TFT_eSPI tft;
const char *ssid = "wifi here";
const char *password = "password here";
int check_menu_logo = 0;
int check_setting = 0;

int text_height = 0;
int text_width = 0;

//star
#define NSTARS 1024
uint8_t sx[NSTARS] = {};
uint8_t sy[NSTARS] = {};
uint8_t sz[NSTARS] = {};
uint8_t za, zb, zc, zx;
uint8_t __attribute__((always_inline)) rng()
{
  zx++;
  za = (za ^ zc ^ zx);
  zb = (zb + za);
  zc = ((zc + (zb >> 1)) ^ za);
  return zc;
}

void star_bg()
{
  unsigned long t0 = micros();
  uint8_t spawnDepthVariation = 255;

  for (int i = 0; i < NSTARS; ++i)
  {
    if (sz[i] <= 1)
    {
      sx[i] = 160 - 120 + rng();
      sy[i] = rng();
      sz[i] = spawnDepthVariation--;
    }
    else
    {
      int old_screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
      int old_screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

      // This is a faster pixel drawing function for occassions where many single pixels must be drawn
      tft.drawPixel(old_screen_x, old_screen_y, TFT_BLACK);

      sz[i] -= 2;
      if (sz[i] > 1)
      {
        int screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
        int screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

        if (screen_x >= 0 && screen_y >= 0 && screen_x < 320 && screen_y < 240)
        {
          uint8_t r, g, b;
          r = g = b = 255 - sz[i];
          tft.drawPixel(screen_x, screen_y, tft.color565(r, g, b));
        }
        else
          sz[i] = 0; // Out of screen, die.
      }
    }
  }
  unsigned long t1 = micros();
  //static char timeMicros[8] = {};
}

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

  tft.fillScreen(MEE_GREYPURPLE);
  tft.setFreeFont(&FreeSansBoldOblique24pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("Mee Plan", 60, 100);

  while (check_menu_logo != 1)
  {
    tft.setFreeFont(&FreeSansBoldOblique12pt7b);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.drawString("click to continue..", 60, 180);
    text_height = tft.fontHeight();
    text_width = tft.textWidth("click to continue..");
    tft.fillRect(60, 180, text_width, text_height, MEE_GREYPURPLE);
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
    if (digitalRead(WIO_5S_LEFT) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_5S_RIGHT) == LOW)
    {
      check_menu_logo = 1;
    }
    if (digitalRead(WIO_5S_PRESS) == LOW)
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
  tft.fillScreen(MEE_GREYPURPLE);
  tft.setFreeFont(&FreeSansBoldOblique18pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("> Connect WIFI", 25, 20);
  tft.drawString("\" Mee Plan \"", 60, 100);
  while (check_setting != 1)
  {
    tft.setFreeFont(&FreeSansBoldOblique12pt7b);
    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(1);
    tft.drawString("waiting for connection..", 25, 180);
    text_height = tft.fontHeight();
    text_width = tft.textWidth("waiting for connection..");
    tft.fillRect(25, 180, text_width, text_height, MEE_GREYPURPLE);
    delay(800);
    tft.drawString("waiting for connection..", 25, 180);
    delay(800);
  }
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
  za = random(256);
  zb = random(256);
  zc = random(256);
  zx = random(256);

  tft.fillScreen(TFT_BLACK);
  drawSetting();
}

void loop()
{
  delay(1000);
}
