// MeePlanTerminal.cpp

#define _WEBSOCKETS_LOGLEVEL_ 3

#define WEBSOCKETS_NETWORK_TYPE NETWORK_RTL8720DN

#define BOARD_TYPE "SAMD SEEED_WIO_TERMINAL"

#include <ArduinoJson.h>
#include <rpcWiFi.h>
#include <rpcping.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>
#include <Bounce2.h>
#include "TFT_eSPI.h"
#include <RTC_SAMD51.h>
#include <DateTime.h>
#define FLASH_DEBUG 1
#include <FlashStorage_SAMD.h>

#include "icon.h"

#include "Free_Fonts.h"

const int WRITTEN_SIGNATURE = 133735;
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
typedef enum tasktype
{
  OFF,
  BLUE,
  GREEN,
  YELLOW,
  ORANGE,
  RED,
  GREY,
  BLACK,
} task_type;

class Task
{
private:
  char msgln1[22];
  char msgln2[22];
  char dueText[33];
  String id = "";
  task_type type = OFF;
  int status = 2;

public:
  char *getNameLine1() { return msgln1; }
  char *getNameLine2() { return msgln2; }
  char *getDueText() { return dueText; };
  task_type getType() { return type; };
  int getStatus() { return status; };
  String getID() { return id; };
  void setType(task_type tasktype) { this->type = tasktype; };
  void setStatus(int taskstatus) { this->status = taskstatus; };
  void setText(const char msg1[], const char msg2[], const char due[]);
  void setID(String ID) { this->id = String(ID); };
  Task()
  {
    memset(msgln1, '\0', sizeof(msgln1));
    memset(msgln2, '\0', sizeof(msgln2));
    memset(dueText, '\0', sizeof(dueText));
  };
};

void Task::setText(const char msg1[22], const char msg2[22], const char due[])
{
  memcpy(this->msgln1, msg1, sizeof(msgln1));
  memcpy(this->msgln2, msg2, sizeof(msgln2));
  memcpy(this->dueText, due, sizeof(dueText));
}

Bounce *buttons = new Bounce[NUM_BUTTONS];
WiFiManager wifiManager;
TFT_eSPI tft;
RTC_SAMD51 rtc;
PingClass internet_test;
SocketIOclient socketIO;

bool check_menu_logo = false;
bool socket_connect = false;
unsigned long tick_now = 0;

int text_height = 0;
int text_width = 0;

const uint16_t iconWidth = 28;
const uint16_t iconHeight = 28;

//task storiung
int task_count = 4;
int pg_count = 0;
int current_pg = 1;

Task taskList[4] = {Task(), Task(), Task(), Task()};

void testTask()
{
  char *taskmsg1[4] = {"TEST MSG LINE ONE ^_^", "TEST MSG TWO ONE  T_T", "TEST MSG THREE ONE^_^", "I'M POOR"};
  char *taskmsg2[4] = {"TEST MSG LINE TWO T_T", "TEST MSG TWO TWO  ^_^", "TEST MSG THREE TWOT_T", "GIVE ME MONEY"};
  char *taskdue[4] = {"15/12/2021 00:00", "15/12/2021 01:00", "15/12/2021 02:00", "69/96/2021 02:00"};
  int taskstype[4] = {2, 1, 0, 3};
  int tasksstatus[4] = {0, 1, 0, 1};
  for (int i = 0; i < 4; i++)
  {
    taskList[i].setText(taskmsg1[i], taskmsg2[i], taskdue[i]);
    taskList[i].setType(static_cast<task_type>(taskstype[i]));
    taskList[i].setStatus(tasksstatus[i]);
  }
}

String settingtext[4] = {"About MeePlan", "Wifi setting", "Reboot", "Factory Reset"};
String settingtext2[4] = {"", "", "", ""};

bool is_draw = false;
bool is_draw_top = false;
int cursor_position = 0;
enum action current_action = NONE;
enum mode current_mode = TASK;

//network related address and port
unsigned int local_port = 2390;
char time_server[] = "1.th.pool.ntp.org";
const int NTP_PACKET_SIZE = 48;
byte packet_buffer[NTP_PACKET_SIZE];
DateTime now;
WiFiUDP udp;
const char server_IP[] = "api.pannanap.pw";
const uint16_t server_port = 80;
unsigned long device_time;
char clock_text[] = "hh:mm";

//for storing data on flash
typedef struct
{
  char id[7];
} DeviceSetting;
DeviceSetting currentSetting;

int signature;
int storedAddress = 0;

void fillMenu(uint32_t color)
{
  tft.fillRect(0, 32, SCREEN_WIDTH, SCREEN_HEIGHT - 32, color);
}

void editTask(int id, int status)
{
  DynamicJsonDocument temp(1024);
  JsonArray array = temp.to<JsonArray>();
  String output;
  array[0] = "edit_iot";
  array[1]["id"] = taskList[id].getID();
  array[1]["done"] = status;
  serializeJson(array, output);
  Serial.println(output);
  socketIO.sendEVENT(output);
}

void updateTaskPage(DynamicJsonDocument &payloadarray)
{ //[event, {}]
  JsonObject tasksobject = payloadarray[1].as<JsonObject>();
  if (tasksobject.isNull())
  {
    return;
  }
  task_count = tasksobject["s"];
  current_pg = tasksobject["pg"];
  pg_count = tasksobject["pgcount"];
  JsonArray tasksdata = tasksobject["data"].as<JsonArray>();
  if (tasksobject.isNull())
  {
    return;
  }
  for (int i = 0; i < task_count; i++)
  {
    const char *msg1 = tasksdata[i][4];
    const char *msg2 = tasksdata[i][5];
    const char *due = tasksdata[i][3];
    const char *objectID = tasksdata[i][0];
    Serial.println(objectID);
    taskList[i].setText(msg1, msg2, due);
    taskList[i].setType(static_cast<task_type>(tasksdata[i][1].as<int>()));
    taskList[i].setID(objectID);
    taskList[i].setStatus(tasksdata[i][2]);
  }
  for (int i = 3; i > task_count; i--)
  {
    const char *msg1 = "";
    const char *msg2 = "";
    const char *due = "";
    const char *objectID = "";
    taskList[i].setText(msg1, msg2, due);
    taskList[i].setType(OFF);
    taskList[i].setID(objectID);
    taskList[i].setStatus(0);
  }
  is_draw = 0;
  //serializeJson(tasksobject,Serial);
}

void displayAlarm(DynamicJsonDocument &payloadarray)
{
  is_draw = 0;
  is_draw_top = 0;
  JsonObject tasksobject = payloadarray[1].as<JsonObject>();
  if (tasksobject.isNull())
  {
    return;
  }
  const char *alarmName = tasksobject["name"];
  const char *alarmDescription = tasksobject["description"];
  const char *alarmDate = tasksobject["date"];
  bool seeAlarm = false;
  bool blink = false;
  tft.setTextFont(1);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(MEE_GREYPURPLE);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(4);
  tft.drawString("Alarm", SCREEN_WIDTH / 2, 10);
  tft.setTextSize(2);
  tft.drawString(alarmName, SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) - 60);
  tft.drawString(alarmDescription, SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) - 20);
  tft.drawString(alarmDate, SCREEN_WIDTH / 2, (SCREEN_HEIGHT / 2) + 20);
  int volume = 128;
  int sec=1;
  while (!seeAlarm)
  {
    socketIO.loop();
    text_height = tft.fontHeight();
    text_width = tft.textWidth("Click to continue");
    if (millis() - tick_now > 500)
    {
      if (blink)
      {
        tft.fillRect(55, 180, text_width + 10, text_height, MEE_GREYPURPLE);
        analogWrite(WIO_BUZZER, 0);
      }
      else
      {
        tft.drawString("Click to continue", SCREEN_WIDTH / 2, 180);
        analogWrite(WIO_BUZZER, volume);
      }
      
      if(volume>10)
      {
        volume-=1;
      }
      sec+=1;
      if(sec>=120)
      {
        seeAlarm = true;
        analogWrite(WIO_BUZZER, 0);
      }

      blink = !blink;

      tick_now = millis();
    }

    


    for (int i = 0; i < NUM_BUTTONS; i++)
    {
      buttons[i].update();
      if (buttons[i].fell())
      {
        analogWrite(WIO_BUZZER, 0);
        seeAlarm = true;
      }
    }
  }
}

void clearTask()
{
  for (int i = 0; i < task_count; i++)
  {
    const char *msg1 = "";
    const char *msg2 = "";
    const char *due = "";
    const char *objectID = "";
    taskList[i].setText(msg1, msg2, due);
    taskList[i].setType(OFF);
    taskList[i].setID(objectID);
    taskList[i].setStatus(0);
  }
  task_count = 0;
}

void sendListiot(int pgnumber)
{
  DynamicJsonDocument temp(4096);
  String output;
  JsonArray array = temp.to<JsonArray>();
  array.add("list_iot");
  array[1]["pg"] = pgnumber;
  serializeJson(array, output);
  Serial.println(output);
  socketIO.sendEVENT(output);
}

void serverEvent(uint8_t *payload, size_t length)
{
  DynamicJsonDocument doc(4096);
  deserializeJson(doc, (char *)payload);
  const char *event = doc[0];
  Serial.println(event);
  if (strcmp(event, "update") == 0)
  {
    sendListiot(current_pg);
  }
  else if (strcmp(event, "list_iot") == 0)
  {
    updateTaskPage(doc);
  }
  else if (strcmp(event, "alarm") == 0)
  {
    displayAlarm(doc);
  }
  else
  {
    Serial.println("no match");
  }
}

void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case sIOtype_DISCONNECT:
    Serial.println("[IOc] Disconnected");
    break;
  case sIOtype_CONNECT:
    Serial.print("[IOc] Connected to url: ");
    Serial.println((char *)payload);

    // join default namespace (no auto join in Socket.IO V3)
    socketIO.send(sIOtype_CONNECT, "/");

    break;
  case sIOtype_EVENT:
    Serial.print("[IOc] Get event: ");
    Serial.println((const char *)payload);
    serverEvent(payload, length);
    break;
  case sIOtype_ACK:
    Serial.print("[IOc] Get ack: ");
    Serial.println(length);

    //hexdump(payload, length);
    break;
  case sIOtype_ERROR:
    Serial.print("[IOc] Get error: ");
    Serial.println(length);

    //hexdump(payload, length);
    break;
  case sIOtype_BINARY_EVENT:
    Serial.print("[IOc] Get binary: ");
    Serial.println(length);

    //hexdump(payload, length);
    break;
  case sIOtype_BINARY_ACK:
    Serial.print("[IOc] Get binary ack: ");
    Serial.println(length);

    //hexdump(payload, length);
    break;

  default:
    break;
  }
}

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
  tft.drawString(">", 10, 50 + (cursor_position * 46));
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

void generateDeviceID(char DeviceID[7])
{
  for (int i = 0; i < 6; i++)
  {
    DeviceID[i] = (char)random(65, 90);
  }
  DeviceID[6] = '\0';
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
  tft.drawString("MeePlan", 60, 85);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.drawString("Device ID: ", 75, 150);
  tft.drawString(currentSetting.id, 170, 150);
  check_menu_logo = false;
  while (!check_menu_logo)
  {
    tft.setFreeFont(&FreeSansBoldOblique12pt7b);
    tft.setTextColor(TFT_LIGHTGREY);
    text_height = tft.fontHeight();
    text_width = tft.textWidth("click to continue");
    if (millis() - tick_now > 500)
    {
      if (blink)
      {
        tft.fillRect(60, 180, text_width + 10, text_height, MEE_GREYPURPLE);
      }
      else
      {
        tft.drawString("Click to continue", 60, 180);
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

void MeePlan_Setup()
{
  bool blink = false;
  int state = 0;
  bool draw = false;
  tick_now = millis();
  tft.fillScreen(MEE_GREYPURPLE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);

  while (state <= 6)
  {
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
      buttons[i].update();
      if (buttons[i].fell())
      {
        Serial.println(state);
        state = state + 1;
        draw = false;
        tft.fillScreen(MEE_GREYPURPLE);
      }
    }
    if (!draw)
    {
      switch (state)
      {
      case 0:
        tft.setFreeFont(&FreeSansBoldOblique24pt7b);
        tft.drawString("MeePlan", 60, 85);
        tft.setFreeFont(&FreeSansBold9pt7b);
        //tft.drawString("Device ID: ", 75, 150);
        //tft.drawString(id, 170, 150);
        break;
      case 1:
        generateDeviceID(currentSetting.id);
        EEPROM.put(storedAddress, WRITTEN_SIGNATURE);
        EEPROM.put(storedAddress + sizeof(signature), currentSetting);
        if (!EEPROM.getCommitASAP())
        {
          Serial.println("CommitASAP not set. Need commit()");
          EEPROM.commit();
        }
        tft.setTextDatum(TC_DATUM);
        tft.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.drawString("Enter device ID in", SCREEN_WIDTH/2, 50);
        tft.drawString("MeePlan Web setting.", SCREEN_WIDTH/2, 75);
        tft.setTextDatum(TL_DATUM);
        tft.setFreeFont(&FreeSansBold9pt7b);
        tft.drawString("Device ID: ", 75, 150);
        tft.drawString(currentSetting.id, 170, 150);
        break;
      case 2:
        tft.setTextDatum(TC_DATUM);
        tft.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.drawString("You can view device ID", SCREEN_WIDTH/2, 50);
        tft.drawString("in MeePlan Setting.", SCREEN_WIDTH/2, 75);
        tft.pushImage((SCREEN_WIDTH/2)- (iconWidth/2), 110, iconWidth, iconHeight, settingicon);
        tft.setFreeFont(&FreeSansBold9pt7b);
        tft.drawString("(Restart after enter on website)", SCREEN_WIDTH/2, 150);
        
        break;
      case 3:
        tft.setTextDatum(TC_DATUM);
        tft.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.drawString("Up Down", SCREEN_WIDTH/2, 50);
        tft.drawString("to navigate.", SCREEN_WIDTH/2, 75);
        tft.drawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2,10, TFT_BLUE);
        tft.fillCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2,10, TFT_BLUE);
        
        break;
      case 4:
        tft.setTextDatum(TC_DATUM);
        tft.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.drawString("Left Right", SCREEN_WIDTH/2, 50);
        tft.drawString("to change page.", SCREEN_WIDTH/2, 75);
        tft.drawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2,10, TFT_BLUE);
        tft.fillCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2,10, TFT_BLUE);
        tft.setFreeFont(&FreeSansBold9pt7b);
        tft.drawString("(In task screen only)", SCREEN_WIDTH/2, 150);

        break;
      case 5:
        tft.setTextDatum(TC_DATUM);
        tft.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.drawString("Push in", SCREEN_WIDTH/2, 50);
        tft.drawString("to do action.", SCREEN_WIDTH/2, 75);
        tft.drawCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2,10, TFT_BLUE);
        tft.fillCircle(SCREEN_WIDTH/2, SCREEN_HEIGHT/2,10, TFT_BLUE);
        tft.setFreeFont(&FreeSansBold9pt7b);
        break;
      case 6:
        tft.setTextDatum(TC_DATUM);
        tft.setFreeFont(&FreeSansBoldOblique12pt7b);
        tft.drawString("Push buttons on top", SCREEN_WIDTH/2, 50);
        tft.drawString("to change menu.", SCREEN_WIDTH/2, 75);
        tft.drawString("To do, Clock, Setting", SCREEN_WIDTH/2, 140);
        tft.setTextDatum(TL_DATUM);
        tft.drawString("^     ^     ^", 30, 10);
        tft.pushImage((SCREEN_WIDTH/2)- (iconWidth/2)*3, 110, iconWidth, iconHeight, taskicon);
        tft.pushImage((SCREEN_WIDTH/2)- (iconWidth/2), 110, iconWidth, iconHeight, clockicon);
        tft.pushImage((SCREEN_WIDTH/2)+ (iconWidth/2), 110, iconWidth, iconHeight, settingicon);
        
        tft.setFreeFont(&FreeSansBold9pt7b);
        break;
      default:
        break;
      }
      draw = true;
    }
    tft.setTextDatum(TL_DATUM);
    tft.setFreeFont(&FreeSansBoldOblique12pt7b);
    tft.setTextColor(TFT_LIGHTGREY);
    text_height = tft.fontHeight();
    text_width = tft.textWidth("Click to continue");
    if (millis() - tick_now > 500)
    {
      if (blink)
      {
        tft.fillRect(60, 180, text_width + 10, text_height, MEE_GREYPURPLE);
      }
      else
      {
        tft.drawString("Click to continue", 60, 180);
      }
      blink = !blink;

      tick_now = millis();
    }
  }
  tft.setTextFont(1);
  tft.setTextSize(2);
}

void drawTask(uint32_t x, uint32_t y, task_type type, int status, const char *msg1, const char *msg2, const char *due)
{
  tft.setTextFont(1);
  tft.setTextColor(TFT_BLACK, MEE_LIGHTPURPLE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(2);
  switch (type)
  {
  case BLUE:
    tft.setTextColor(TFT_BLACK, TFT_CYAN);
    drawSelectbox(x, y, TFT_CYAN);
    break;
  case GREEN:
    tft.setTextColor(TFT_BLACK, TFT_GREENYELLOW);
    drawSelectbox(x, y, TFT_GREENYELLOW);
    break;
  case YELLOW:
    tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    drawSelectbox(x, y, TFT_YELLOW);
    break;
  case ORANGE:
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    drawSelectbox(x, y, TFT_ORANGE);
    break;
  case RED:
    tft.setTextColor(TFT_WHITE, TFT_RED);
    drawSelectbox(x, y, TFT_RED);
    break;
  case GREY:
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    drawSelectbox(x, y, TFT_DARKGREY);
    break;
  case BLACK:
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    drawSelectbox(x, y, TFT_BLACK);
    break;
  case OFF:
    break;
  }
  if (type != OFF)
  {
    tft.drawString(msg1, x + 5, y + 10);
    tft.drawString(msg2, x + 5, y + 27);
    tft.setTextDatum(TR_DATUM);
    tft.setTextSize(1);
    tft.drawString(due, x + 255, y + 1);

    tft.setTextColor(TFT_BLACK, TFT_WHITE);
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
      tft.drawString("/", 295, y + 15);
    }
  }
  tft.setTextDatum(TL_DATUM);
}

void drawTask(uint32_t x, uint32_t y, Task task)
{
  drawTask(x, y, task.getType(), task.getStatus(), task.getNameLine1(), task.getNameLine2(), task.getDueText());
}

void drawPageBar()
{
  if (pg_count != 0)
  {
    String temppg;
    temppg += current_pg;
    String temppgcount;
    temppgcount += pg_count;
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(TFT_BLACK, MEE_LIGHTPURPLE);
    tft.drawString("/", (SCREEN_WIDTH / 2), SCREEN_HEIGHT - 10);
    tft.setTextSize(2);
    if (current_pg != 1)
      tft.drawString("<-", (SCREEN_WIDTH / 2) - 65, SCREEN_HEIGHT - 10);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(temppg, (SCREEN_WIDTH / 2) - 30, SCREEN_HEIGHT - 10);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(temppgcount, (SCREEN_WIDTH / 2) + 30, SCREEN_HEIGHT - 10);
    if (current_pg != pg_count)
    {
      tft.setTextDatum(TC_DATUM);
      tft.drawString("->", (SCREEN_WIDTH / 2) + 65, SCREEN_HEIGHT - 10);
    }

    //tft.drawString("/",SCREEN_WIDTH/2,SCREEN_HEIGHT-10);
  }
}

void wifiConnect(const char *ssid)
{
  tft.setTextDatum(TL_DATUM);
  while (!WiFi.isConnected())
  {
    tft.fillScreen(MEE_GREYPURPLE);
    tft.setFreeFont(&FreeSansBoldOblique18pt7b);
    tft.setTextColor(TFT_WHITE);
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
}

void serverConnect()
{
  tft.setTextDatum(TL_DATUM);

  tft.fillScreen(MEE_GREYPURPLE);
  tft.setFreeFont(&FreeSansBoldOblique18pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("Connecting...", 160, 20);
  tft.setFreeFont(&FreeSansBoldOblique12pt7b);
  delay(500);
  socketIO.loop();
}

void setupScreen(uint32_t color, enum mode mode)
{
  switch (mode)
  {
  default:
    fillMenu(color);
    tft.drawRect(0, 0, SCREEN_WIDTH - 70, 32, TFT_LIGHTGREY);
    tft.fillRect(0, 0, SCREEN_WIDTH - 70, 32, TFT_LIGHTGREY);
  case CLOCK:
    fillMenu(color);
    tft.drawRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
    tft.fillRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
    break;
  }
  tft.pushImage(2, 2, iconWidth, iconHeight, taskicon);
  tft.pushImage(30, 2, iconWidth, iconHeight, clockicon);
  tft.pushImage(58, 2, iconWidth, iconHeight, settingicon);
}
void setupScreen(uint32_t color)
{
  fillMenu(color);
  tft.drawRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
  tft.fillRect(0, 0, SCREEN_WIDTH, 32, TFT_LIGHTGREY);
}

void handleTaskAction();
void handleOptionAction();

void setup()
{
  pinMode(WIO_BUZZER, OUTPUT);
  char ssidapname[16] = "MeePlanTerm";
  const char *ssidap = ssidapname;
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
  randomSeed(now.unixtime());
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

  //generateDeviceID(id);
  Serial.println(currentSetting.id);
  //testTask();

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP); //setup the bounce instance for the current button
    buttons[i].interval(20);
  }
  EEPROM.get(storedAddress, signature);
  EEPROM.get(storedAddress+sizeof(signature), currentSetting);
  if (signature != WRITTEN_SIGNATURE)
  {
    MeePlan_Setup();
  }
  socketIO.setReconnectInterval(10000);
  String auth = "Authorization: ";
  auth += currentSetting.id;
  socketIO.setExtraHeaders(auth.c_str());
  socketIO.begin(server_IP, server_port);
  socketIO.onEvent(socketIOEvent);
  serverConnect();
  setupScreen(MEE_LIGHTPURPLE);
}

void loop()
{
  if (!socketIO.isConnected())
  {
    tft.setTextFont(1);
    tft.setTextSize(1.5);
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    tft.setTextDatum(TC_DATUM);
    tft.drawString("NOT CONNECTED", 160, 2);
    if (task_count != 0)
    {
      clearTask();
      is_draw = false;
      is_draw_top = false;
    }
    socket_connect = false;
  }
  else if (!socket_connect)
  {
    is_draw = false;
    is_draw_top = false;
    socket_connect = true;
  }
  if (WiFi.isConnected())
  {
    socketIO.loop();
  }
  tft.setTextSize(5);
  now = rtc.now();
  updateKey();
  //structure for menu
  switch (current_mode)
  {
  case TASK:
    if (!is_draw)
    {
      tft.setTextFont(1);
      tft.setTextDatum(TL_DATUM);
      if (!is_draw_top)
      {
        setupScreen(MEE_LIGHTPURPLE, TASK);
        tft.setTextSize(2);
        tft.setTextDatum(CC_DATUM);
        tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
        tft.drawString("TODO", SCREEN_WIDTH / 2, 18);
        is_draw_top = true;
      }
      fillMenu(MEE_LIGHTPURPLE);
      for (int i = 0; i < task_count; i++)
      {
        drawTask(30, 35 + (i * 46), taskList[i]);
      }
      drawPageBar();
      updateCursor(MEE_LIGHTPURPLE, TFT_BLACK);
      is_draw = true;
    }
    handleTaskAction();
    break;
  case CLOCK:
    if (!is_draw)
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
    if (current_action == ONE)
    {
      current_mode = TASK;
      is_draw = false;
      is_draw_top = false;
    }
    if (current_action == THREE)
    {
      current_mode = SETTING;
      is_draw = false;
      is_draw_top = false;
    }
    break;
  case SETTING:
    if (!is_draw)
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
        settingtext2[1] = "Not Connected!";
      }
      else
      {
        settingtext2[1] = WiFi.SSID();
      }
      Serial.println(settingtext[2]);
      for (int i = 0; i < 4; i++)
      {
        drawTask(30, 35 + (i * 46), BLACK, 2, settingtext[i].c_str(), settingtext2[i].c_str(), "");
      }
      is_draw = true;
      is_draw_top = true;
    }
    handleOptionAction();
    break;
  }
  if (current_mode != CLOCK)
  {
    tft.setTextFont(1);
    tft.setTextSize(2.5);
    tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
    if (WiFi.isConnected())
    {
      strcpy(clock_text, "hh:mm");
      tft.setTextDatum(TR_DATUM);
      tft.drawString(now.toString(clock_text), 310, 10);
    }
  }
}

void handleTaskAction()
{
  switch (current_action)
  {
  case NONE:
    break;
  case ONE:
  {
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();
    array.add("task");
    String output;
    serializeJson(array, output);
    socketIO.sendEVENT(output);
    is_draw = 0;
  }
  break;
  case TWO:
    current_mode = CLOCK;
    cursor_position = 0;
    is_draw_top = false;
    is_draw = false;
    break;
  case THREE:
    current_mode = SETTING;
    cursor_position = 0;
    is_draw_top = false;
    is_draw = false;
    break;
  case UP:
    if (cursor_position == 0)
    {
      cursor_position = 3;
    }
    else if (cursor_position > 0)
    {
      cursor_position--;
    }
    updateCursor(MEE_LIGHTPURPLE, TFT_BLACK);
    break;
  case DOWN:
    if (cursor_position == 3)
    {
      cursor_position = 0;
    }
    else if (cursor_position < 3)
    {
      cursor_position++;
    }
    updateCursor(MEE_LIGHTPURPLE, TFT_BLACK);
    break;
  case LEFT:
    if (current_pg > 1)
    {
      current_pg--;
      sendListiot(current_pg);
    }
    break;
  case RIGHT:
    if (current_pg != pg_count)
    {
      current_pg++;
      sendListiot(current_pg);
    }
    break;
  case PUSH:
    editTask(cursor_position, taskList[cursor_position].getStatus() == 0 ? 1 : 0);
    break;
  }
}

void handleOptionAction()
{
  switch (current_action)
  {
  case NONE:
    break;
  case ONE:
    current_mode = TASK;
    cursor_position = 0;
    is_draw = false;
    is_draw_top = false;
    break;
  case TWO:
    current_mode = CLOCK;
    cursor_position = 0;
    is_draw = false;
    is_draw_top = false;
    break;
  case THREE:
    break;
  case UP:
    if (cursor_position == 0)
    {
      cursor_position = 3;
    }
    else if (cursor_position > 0)
    {
      cursor_position--;
    }
    updateCursor(MEE_GREYPURPLE, TFT_WHITE);
    break;
  case DOWN:
    if (cursor_position == 3)
    {
      cursor_position = 0;
    }
    else if (cursor_position < 3)
    {
      cursor_position++;
    }
    updateCursor(MEE_GREYPURPLE, TFT_WHITE);
    break;
  case LEFT:
    break;
  case RIGHT:
    break;
  case PUSH:
    if (cursor_position == 0)
    {
      if (socketIO.isConnected())
      {
        MeePlan_Logo();
        is_draw = false;
        setupScreen(MEE_GREYPURPLE);
      }
    }
    else if (cursor_position == 1)
    {
      tft.drawCircle(10, 10, 5, TFT_RED);
      wifiManager.resetSettings();
      NVIC_SystemReset();
    }
    else if (cursor_position == 2)
    {
      NVIC_SystemReset();
    }
    else if (cursor_position == 3)
    {
      EEPROM.put(storedAddress, 0);
      if (!EEPROM.getCommitASAP())
      {
        Serial.println("CommitASAP not set. Need commit()");
        EEPROM.commit();
      }
      wifiManager.resetSettings();
      NVIC_SystemReset();
    }
    break;
  }
}