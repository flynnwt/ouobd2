// Websocket server
// ESP8266WebSockets also defines WebsocketServer!  Should
//  probably namespace this one?

#ifndef _WEBSOCKETSERVER_h
#define _WEBSOCKETSERVER_h

#include <ESP8266WiFi.h>
#include <ESP8266WebSockets.h>

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

class WebsocketServer {
  unsigned int port;
  WebSocketServer *socket;
  WiFiServer *server;
  WiFiClient client;
  unsigned long lastSend = 0;
  void(*cbConnect)() = NULL;
  void(*cbDisconnect)() = NULL;
  void(*cbReceive)(String data) = NULL;

public:
  bool connected = false;
  WebsocketServer(unsigned int port);
  void setConnect(void (cb)());
  void setDisconnect(void (cb)());
  void setReceive(void (cb)(String data));
  
  void begin();
  void process();
  void send(String data);

};

#endif

