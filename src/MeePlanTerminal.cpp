//Modifed from HTTPS GET Example
//to call a REST api and displaying data on WIO Terminal's screen.

#include <ArduinoJson.h>
#include <rpcWiFi.h>
#include <rpcping.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Bounce2.h>
#include "TFT_eSPI.h"
#include <RTC_SAMD51.h>
#include <DateTime.h>

#include "Free_Fonts.h"

//tft color format rgb565
#define MEE_GREYPURPLE 0x526B
#define MEE_LIGHTPURPLE 0xE71F

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define NUM_BUTTONS 8
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {WIO_KEY_C, WIO_KEY_B, WIO_KEY_A, WIO_5S_PRESS, WIO_5S_UP, WIO_5S_RIGHT, WIO_5S_DOWN, WIO_5S_LEFT};
enum mode
{
  TASK,
  CLOCK,
  SETTING,
};
enum action
{
  ONE,
  TWO,
  THREE,
  PUSH,
  UP,
  RIGHT,
  DOWN,
  LEFT,
  NONE,
};

Bounce *buttons = new Bounce[NUM_BUTTONS];
WiFiManager wifiManager;
TFT_eSPI tft;
RTC_SAMD51 rtc;
PingClass internet_test;

bool check_menu_logo = false;
unsigned long tick_now = 0;

int text_height = 0;
int text_width = 0;

//for testing
int task_count = 4;
int pg_count = 1;
int current_pg = 1;

String taskmsg1[4] = {"TEST MSG LINE ONE ^_^", "TEST MSG TWO ONE  T_T", "TEST MSG THREE ONE^_^", "I'M POOR"};
String taskmsg2[4] = {"TEST MSG LINE TWO T_T", "TEST MSG TWO TWO  ^_^", "TEST MSG THREE TWOT_T", "GIVE ME MONEY"};
String taskdue[4] = {"15/12/2021 00:00", "15/12/2021 01:00", "15/12/2021 02:00", "69/96/2021 02:00"};

String settingtext[4] = {"About MeePlan", "Reset Wifi setting", "Current Wifi: ", "Reboot"};
String settingtext2[4] = {"", "", "", ""};

int taskstype[4] = {2, 1, 0, 3};
int tasksstatus[4] = {0, 1, 0, 1};

bool is_draw = false;
int is_draw_top = 0;
int cursor_position = 0;
enum action current_action = NONE;
enum mode current_mode = TASK;

unsigned int local_port = 2390;
char time_server[] = "1.th.pool.ntp.org";
const int NTP_PACKET_SIZE = 48;
byte packet_buffer[NTP_PACKET_SIZE];
DateTime now;
WiFiUDP udp;
unsigned long device_time;
char clock_text[] = "hh:mm";


/*
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
*/
unsigned long sendNTPpacket(const char *address)
{
  for (int i = 0; i < NTP_PACKET_SIZE; ++i)
  {
    packet_buffer[i] = 0;
  }
  packet_buffer[0] = 0b11100011;
  packet_buffer[1] = 0;
  packet_buffer[2] = 6;
  packet_buffer[3] = 0xEC;
  packet_buffer[12] = 49;
  packet_buffer[13] = 0x4E;
  packet_buffer[14] = 49;
  packet_buffer[15] = 52;
  udp.beginPacket(address, 123);
  udp.write(packet_buffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

unsigned long getNTPtime()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    udp.begin(WiFi.localIP(), local_port);
    sendNTPpacket(time_server);
    delay(1000);
    if (udp.parsePacket())
    {
      udp.read(packet_buffer, NTP_PACKET_SIZE);
      unsigned long highWord = word(packet_buffer[40], packet_buffer[41]);
      unsigned long lowWord = word(packet_buffer[42], packet_buffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;
      long tzOffset = 25200UL;
      unsigned long adjustedTime;
      return adjustedTime = epoch + tzOffset;
    }
    else
    {
      udp.stop();
      return 0;
    }
    udp.stop();
  }
  else
  {
    return 0;
  }
}

void drawSelectbox(int32_t x, int32_t y, uint32_t color)
{
  tft.drawRoundRect(x, y, 260, 45, 5, color);
  tft.fillRoundRect(x, y, 260, 45, 5, color);
}

void fillMenu(uint32_t color)
{
  tft.fillRect(0, 32, SCREEN_WIDTH, SCREEN_HEIGHT - 32, color);
}

void drawArrows()
{
  tft.drawTriangle(270, 80, 290, 40, 310, 80, TFT_DARKGREY);
  tft.fillTriangle(270, 80, 290, 40, 310, 80, TFT_DARKGREY);
  tft.drawTriangle(270, 190, 290, 230, 310, 190, TFT_DARKGREY);
  tft.fillTriangle(270, 190, 290, 230, 310, 190, TFT_DARKGREY);
}

void updateCursor(uint32_t color, uint32_t cs_color)
{
  tft.fillRect(0, 32, 25, SCREEN_HEIGHT - 32, color);
  tft.setTextFont(1);
  tft.setTextColor(cs_color);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(2);
  tft.drawString(">", 10, 50 + (cursor_position * 50));
}

void updateKey()
{
  current_action = NONE;
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    buttons[i].update();
    if (buttons[i].fell())
    {
      Serial.printf("%d fell.\n", i);
      current_action = static_cast<action>(i);
    }
  }
}

void MeePlan_Logo()
{
  int blink = 0;
  tick_now = millis();
  tft.fillScreen(MEE_GREYPURPLE);
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&FreeSansBoldOblique24pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.drawString("Mee Plan", 60, 100);
  check_menu_logo = false;
  while (check_menu_logo != true)
  {
    tft.setFreeFont(&FreeSansBoldOblique12pt7b);
    tft.setTextColor(TFT_LIGHTGREY);
    text_height = tft.fontHeight();
    text_width = tft.textWidth("click to continue..");
    if (millis() - tick_now > 500)
    {
      if (blink)
      {
        tft.fillRect(60, 180, text_width, text_height, MEE_GREYPURPLE);
      }
      else
      {
        tft.drawString("click to continue..", 60, 180);
      }
      blink = !blink;

      tick_now = millis();
    }
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
      buttons[i].update();
      if (buttons[i].fell())
      {
        check_menu_logo = true;
      }
    }
  }
  tft.setTextFont(1);
  tft.setTextSize(2);
}

void drawTask(uint32_t x, uint32_t y, int type, int status, const char *msg1, const char *msg2, const char *due)
{
  tft.setTextFont(1);
  tft.setTextColor(TFT_BLACK, MEE_LIGHTPURPLE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(2);

  if (status == 0)
  {
    tft.fillRect(293, y + 12, 15, 20, TFT_WHITE);
    tft.drawRect(293, y + 12, 15, 20, TFT_BLACK);
  }
  else if (status == 1)
  {
    tft.fillRect(293, y + 12, 15, 20, TFT_WHITE);
    tft.drawRect(293, y + 12, 15, 20, TFT_BLACK);
    tft.drawString("X", 295, y + 15);
  }

  switch (type)
  {
  case 0:
    tft.setTextColor(TFT_BLACK, TFT_GREENYELLOW);
    drawSelectbox(x, y, TFT_GREENYELLOW);
    break;
  case 1:
    tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    drawSelectbox(x, y, TFT_YELLOW);
    break;
  case 2:
    tft.setTextColor(TFT_WHITE, TFT_RED);
    drawSelectbox(x, y, TFT_RED);
    break;
  case 3:
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    drawSelectbox(x, y, TFT_DARKGREY);
    break;
  default:
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    drawSelectbox(x, y, TFT_BLACK);
    break;
  }

  tft.drawString(msg1, x + 5, y + 10);
  tft.drawString(msg2, x + 5, y + 27);

  tft.setTextDatum(TR_DATUM);
  tft.setTextSize(1);
  tft.drawString(due, x + 255, y + 1);

  tft.setTextDatum(TL_DATUM);
}

void wifiConnect(const char *ssid)
{
  tft.setTextDatum(TL_DATUM);
  while (!WiFi.isConnected())
  {
    tft.fillScreen(MEE_GREYPURPLE);
    tft.setFreeFont(&FreeSansBoldOblique18pt7b);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);

    tft.drawString("> Connect WIFI", 25, 20);
    tft.setFreeFont(&FreeSansBoldOblique12pt7b);
    tft.drawString("\" MeePlanTerm \"", 55, 100);

    tft.setTextColor(TFT_GREEN);
    tft.setTextSize(1);
    tft.drawString("waiting for connection..", 25, 180);
    wifiManager.autoConnect(ssid);
    if (!internet_test.ping("8.8.8.8"))
    {
      continue;
    }
  }
  settingtext2[2] = WiFi.SSID();
}

void setupScreen(uint32_t color, enum mode mode)
{
  switch (mode)
  {
  case CLOCK:
    fillMenu(color);
    tft.drawRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
    tft.fillRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
    break;
  default:
    fillMenu(color);
    tft.drawRect(0, 0, SCREEN_WIDTH - 70, 32, TFT_LIGHTGREY);
    tft.fillRect(0, 0, SCREEN_WIDTH - 70, 32, TFT_LIGHTGREY);
  }
}
void setupScreen(uint32_t color)
{
  fillMenu(color);
  tft.drawRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
  tft.fillRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
}

void setup()
{
  char ssidapname[16] = "MeePlanTerm";
  const char *ssidap = ssidapname;
  /*if(!wifiManager.autoConnect(ssidap)) {
    Serial.println("failed to connect, we should reset as see if it connects");
    delay(3000);
  } */
  rtc.begin();
  tft.begin();
  tft.setRotation(3);
  wifiConnect(ssidap);
  Serial.println(WiFi.SSID());

  rtc.begin();

  
  if (WiFi.isConnected())
  {
    device_time = getNTPtime();
    Serial.println(device_time);
    rtc.adjust(DateTime(device_time));
  }
  now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP); //setup the bounce instance for the current button
    buttons[i].interval(20);
  }
  /*
  za = random(256);
  zb = random(256);
  zc = random(256);
  zx = random(256);
  */

  setupScreen(MEE_LIGHTPURPLE);
}

void loop()
{
  tft.setTextSize(5);
  now = rtc.now();
  updateKey();
  //structure for menu
  switch (current_mode)
  {
  case TASK:
    if (is_draw == false)
    {
      tft.setTextFont(1);
      tft.setTextColor(TFT_BLACK, MEE_LIGHTPURPLE);
      setupScreen(MEE_LIGHTPURPLE, TASK);
      updateCursor(MEE_LIGHTPURPLE, TFT_BLACK);
      tft.setTextSize(2);
      tft.setTextDatum(CC_DATUM);
      tft.drawString("TASK", SCREEN_WIDTH / 2, 18);
      tft.setTextDatum(TL_DATUM);
      for (int i = 0; i < task_count; i++)
      {
        drawTask(30, 35 + (i * 50), taskstype[i], tasksstatus[i], taskmsg1[i].c_str(), taskmsg2[i].c_str(), taskdue[i].c_str());
      }
      is_draw = true;
    }
    if (current_action == TWO)
    {
      current_mode = CLOCK;
      cursor_position = 0;
      is_draw = false;
    }
    if (current_action == UP)
    {
      if (current_pg != 1 && cursor_position == 0)
      {
        current_pg--;
        cursor_position = 3;
      }
      else if (cursor_position > 0)
      {
        cursor_position--;
      }
      updateCursor(MEE_LIGHTPURPLE, TFT_BLACK);
    }
    if (current_action == DOWN)
    {
      if (current_pg != pg_count && cursor_position == 3)
      {
        current_pg--;
        cursor_position = 0;
      }
      else if (cursor_position < 3)
      {
        cursor_position++;
      }
      updateCursor(MEE_LIGHTPURPLE, TFT_BLACK);
    }
    if (current_action == PUSH)
    {
      tasksstatus[cursor_position] = !tasksstatus[cursor_position];
      drawTask(30, 35 + (cursor_position * 50), taskstype[cursor_position], tasksstatus[cursor_position], taskmsg1[cursor_position].c_str(), taskmsg2[cursor_position].c_str(), taskdue[cursor_position].c_str());
    }
    break;
  case CLOCK:

    if (is_draw == false)
    {
      setupScreen(MEE_LIGHTPURPLE, CLOCK);
      tft.setTextSize(2);
      tft.setTextDatum(CC_DATUM);
      tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
      tft.drawString("CLOCK", SCREEN_WIDTH / 2, 18);
      tft.setTextDatum(TL_DATUM);
      is_draw = true;
    }
    else
    {
      tft.setTextFont(1);
      tft.setTextSize(5);
      tft.setTextColor(TFT_BLACK, MEE_LIGHTPURPLE);
      tft.setTextDatum(TL_DATUM);
      tft.drawString(now.timestamp(DateTime::timestampOpt::TIMESTAMP_TIME), 42, 100);
      tft.setTextSize(2);
      tft.drawString(now.timestamp(DateTime::timestampOpt::TIMESTAMP_DATE), 42, 140);
    }

    if (current_action == TWO)
    {
      current_mode = TASK;
      is_draw = false;
    }
    if (current_action == THREE)
    {
      current_mode = SETTING;
      is_draw = false;
    }
    break;
  case SETTING:
    if (is_draw == false)
    {
      tft.setTextFont(1);
      tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
      setupScreen(MEE_GREYPURPLE, SETTING);
      tft.setTextSize(2);
      tft.setTextDatum(CC_DATUM);
      tft.drawString("SETTING", SCREEN_WIDTH / 2, 18);
      tft.setTextDatum(TL_DATUM);
      updateCursor(MEE_GREYPURPLE, TFT_WHITE);
      if (!WiFi.isConnected())
      {
        settingtext[2] = "Not Connected!";
      }
      else
      {
        settingtext[2] = "Connected";
      }
      Serial.println(settingtext[2]);
      for (int i = 0; i < 4; i++)
      {
        drawTask(30, 35 + (i * 50), 4, 2, settingtext[i].c_str(), settingtext2[i].c_str(), "");
      }
      is_draw = true;
    }
    if (current_action == TWO)
    {
      current_mode = CLOCK;
      cursor_position = 0;
      is_draw = false;
    }
    if (current_action == PUSH)
    {
      //might need to change this if more than 4 menu
      if (cursor_position == 0)
      {
        MeePlan_Logo();
        is_draw = false;
        setupScreen(MEE_GREYPURPLE);
      }
      else if (cursor_position == 1)
      {
        tft.drawCircle(10, 10, 5, TFT_RED);
        wifiManager.resetSettings();
        wifiConnect("MeePlanTerm");
        is_draw = false;
        fillMenu(MEE_GREYPURPLE);
        setupScreen(MEE_GREYPURPLE);
      }
      else if (cursor_position == 2)
      {
        tft.drawCircle(10, 10, 5, TFT_BLUE);
      }
      else if (cursor_position == 3)
      {
        tft.drawCircle(10, 10, 5, TFT_YELLOW);
        NVIC_SystemReset();
      }
    }
    if (current_action == UP)
    {
      if (cursor_position == 0)
      {
        cursor_position = 3;
      }
      else if (cursor_position > 0)
      {
        cursor_position--;
      }
      updateCursor(MEE_GREYPURPLE, TFT_WHITE);
    }
    if (current_action == DOWN)
    {

      if (cursor_position == 3)
      {
        cursor_position = 0;
      }
      else if (cursor_position < 3)
      {
        cursor_position++;
      }
      updateCursor(MEE_GREYPURPLE, TFT_WHITE);
    }
    break;
  }
  if (current_mode != CLOCK)
  {
    tft.setTextFont(1);
    tft.setTextSize(2.5);
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    strcpy(clock_text, "hh:mm");
    tft.setTextDatum(TR_DATUM);
    tft.drawString(now.toString(clock_text), 310, 10);
  }
}
