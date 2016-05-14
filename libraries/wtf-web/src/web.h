#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>  // can this be optional?? (so you don't have to include spiffs if you don't want to serve files??? or will it not be included if addStatic isn't called?

class Web {
  String uploadUri;
  String uploadFilename;
  File uploadFile;
  int uploadError;
  void init(unsigned int port);
  void(*cbUploadComplete)(String filename, int err);

public:
  ESP8266WebServer* server;
  
  Web();
  Web(unsigned int port);

  void addRoot(void(handler)());
  void addRoute(String route, void(handler)());
  void addStatic(String uri, String path);
  void addUpload(String uri);
  void addUpload(String uri, void(handler)());
  void addUploadComplete(void(handler)(String filename, int err));
  void uploader();
};
