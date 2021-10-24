/****************************************************************************************************************************
   WebSocketClientStomp_ENC.ino
  For boards using ENC28J60 Shield/Module
  
  Based on and modified from WebSockets libarary https://github.com/Links2004/arduinoWebSockets
  to support other boards such as  SAMD21, SAMD51, Adafruit's nRF52 boards, etc.
  
  Built by Khoi Hoang https://github.com/khoih-prog/WebSockets_Generic
  Licensed under MIT license
  
  Example for connecting and maintining a connection with a STOMP websocket connection.
  In this example, we connect to a Spring application (see https://docs.spring.io/spring/docs/current/spring-framework-reference/html/websocket.html).
  
  Created on: 25.09.2017
  Author: Martin Becker <mgbckr>, Contact: becker@informatik.uni-wuerzburg.de
  
  String url = "/socket.io/?EIO=3", String protocol = "arduino");
 *****************************************************************************************************************************/

#if ( defined(ARDUINO_SAM_DUE) || defined(__SAM3X8E__) )
  // Default pin 10 to SS/CS
  #define USE_THIS_SS_PIN       10
  #define BOARD_TYPE      "SAM DUE"
#elif ( defined(CORE_TEENSY) )  
  #error You have to use examples written for Teensy
#endif

#ifndef BOARD_NAME
  #define BOARD_NAME    BOARD_TYPE
#endif

#define _WEBSOCKETS_LOGLEVEL_     3
#define WEBSOCKETS_NETWORK_TYPE   NETWORK_ENC28J60

#define SHIELD_TYPE               "ENC28J60 using UIPEthernet Library"

#include <WebSocketsClient_Generic.h>

WebSocketsClient webSocket;

uint8_t mac[6] =  { 0xDE, 0xAD, 0xBE, 0xEF, 0xBE, 0x08 };

// Select the IP address according to your local network
IPAddress clientIP(192, 168, 2, 225);
IPAddress serverIP(192, 168, 2, 222);

const char* ws_host               = "the.host.net";
const int   ws_port               = 80;

// URL for STOMP endpoint.
// For the default config of Spring's STOMP support, the default URL is "/socketentry/websocket".
const char* stompUrl            = "/socketentry/websocket"; // don't forget the leading "/" !!!


// FUNCTIONS

/**
   STOMP messages need to be NULL-terminated (i.e., \0 or \u0000).
   However, when we send a String or a char[] array without specifying
   a length, the size of the message payload is derived by strlen() internally,
   thus dropping any NULL values appended to the "msg"-String.

   To solve this, we first convert the String to a NULL terminated char[] array
   via "c_str" and set the length of the payload to include the NULL value.
*/
void sendMessage(String & msg)
{
  webSocket.sendTXT(msg.c_str(), msg.length() + 1);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length)
{
  switch (type) 
  {
    case WStype_DISCONNECTED:
      Serial.println("[WSc] Disconnected!");
      break;
    case WStype_CONNECTED:
      {
        Serial.print("[WSc] Connected to url: ");
        Serial.println((char *) payload);

        String msg = "CONNECT\r\naccept-version:1.1,1.0\r\nheart-beat:10000,10000\r\n\r\n";
        sendMessage(msg);
      }
      break;
    case WStype_TEXT:
      {
        // #####################
        // handle STOMP protocol
        // #####################

        String text = (char*) payload;
        Serial.print("[WSc] get text: ");
        Serial.println((char *) payload);

        if (text.startsWith("CONNECTED")) 
        {

          // subscribe to some channels

          String msg = "SUBSCRIBE\nid:sub-0\ndestination:/user/queue/messages\n\n";
          sendMessage(msg);
          delay(1000);

          // and send a message

          msg = "SEND\ndestination:/app/message\n\n{\"user\":\"esp\",\"message\":\"Hello!\"}";
          sendMessage(msg);
          delay(1000);
        } 
        else 
        {
          // do something with messages
        }

        break;
      }
    case WStype_BIN:
      Serial.print("[WSc] get binary length: ");
      Serial.println(length);
      
      //hexdump(payload, length);

      // send data to server
       webSocket.sendBIN(payload, length);
      break;
  }
}

void setup()
{
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial);

  Serial.print("\nStart WebSocketClientStomp_ENC on " + String(BOARD_NAME));
  Serial.println(" with " + String(SHIELD_TYPE));
  Serial.println(WEBSOCKETS_GENERIC_VERSION);

  Serial.println("Used/default SPI pinout:");
  Serial.print("MOSI:");
  Serial.println(MOSI);
  Serial.print("MISO:");
  Serial.println(MISO);
  Serial.print("SCK:");
  Serial.println(SCK);
  Serial.print("SS:");
  Serial.println(SS);
  
  /// start the ethernet connection and the server:
  // Use Static IP
  Ethernet.begin(mac, clientIP);
  //Configure IP address via DHCP
  Ethernet.begin(mac);

  Serial.print("WebSockets Client IP address: ");
  Serial.println(Ethernet.localIP());

  // connect to websocket
  webSocket.begin(ws_host, ws_port, stompUrl);
  webSocket.setExtraHeaders(); // remove "Origin: file://" header because it breaks the connection with Spring's default websocket config
  //    webSocket.setExtraHeaders("foo: I am so funny\r\nbar: not"); // some headers, in case you feel funny
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  webSocket.loop();
}
