#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebSockets.h>
#include <FS.h>

#include <ArduinoJson.h>

#include <web.h>
#include <wsserver.h>
#include <obd2.h>

#define DEBUGPRT Serial1     // Serial port for messages
#define OBD_SERIAL Serial    // OBD port (Serial unless debugging)
#define DEBUGLIB false       // Enable serial output from libs

// in config.cpp
extern String ssid;
extern String passphrase;
extern String hhostname;
extern String logFileName;
extern unsigned long sendInterval;
extern unsigned int wsPort;
extern unsigned long commandInterval;
extern int statusPin;

MDNSResponder mdns;
Web *web;
WebsocketServer *ws;
OBD *obd;
File logFile;
//String dev = "ELM327 v1.5";  
String dev = "";
unsigned long lastSend = 0;
unsigned long lastCommand = 0;
String results = "";

void wsConnect() {
  DEBUGPRT.println("WS connected.");
}

void wsDisconnect() {
  DEBUGPRT.println("WS disconnected.");
}

void wsReceive(String data) {
  StaticJsonBuffer<2000> jsonBuffer;
  String results;

  DEBUGPRT.println("Got data: " + data);
  // if json, need to escape quotes, etc. or use parser
  //wsSendClientData("{\"log\":\"server rcvd:" + data + "\"}");
  JsonObject& root = jsonBuffer.parseObject(data);
  String type = root["type"];
  if (type == "cmd") {  // at (elm command), special (init, status, pid-shortcuts like rpm, etc.), obd (numbers)
    String cmd = root["value"];
    //wsSendClientData("{\"log\":\"server cmd:" + cmd + "\"}");
    // a map would be nice...
    // and these macro-type commands should not be allowed to be queued? or they should work if queued
    if (cmd == "status") {
      JsonObject& root = jsonBuffer.createObject();
      JsonObject& status = root.createNestedObject("status");
      status["device"] = obd->device;
      status["result"] = obd->result;
      status["recvTimeout"] = obd->recvTimeout;
      status["searchingTimeout"] = obd->searchingTimeout;
      results = "";
      root.printTo(results);
      wsSend(results);
    } else if (cmd == "init") {
      results = "";
      if (obd->init(dev)) {
        results += "OK! Device=" + obd->device;
      } else {
        results += "Failed! " + obd->result;
      }
      results = "{\"init\":\"" + results + "\"}";
      wsSend(results);
      obd->resultPending = false;   // since 'send' done here instead of loop, need to clear the pending result
      results = "";
    } else if (cmd == "reset") {
      results = "NOT IMPLEMENTED!";
      results = "{\"reset\":\"" + results + "\"}";
      wsSend(results);
      //obd->reset();
    } else if (cmd == "clearlog") {
      obd->logClear();
    } else if (cmd == "getlog") {
      results = "{\"rawlog\":{\"logging\":" + String(obd->logging ? "true" : "false") + ",\"data\":\"" + obd->logJson() + "\"}}";
      wsSend(results);
    } else if (cmd == "mon on") {
      obd->monitor(true);
    } else if (cmd == "mon off") {
      obd->monitor(false);
    } else if (isDigit(cmd[0])) {   // obd - should check for dd dd
      //results = "pid(" + cmd.substring(0,2) + "," + cmd.substring(3,5) + "): ";
      obd->command(cmd, "pid(" + cmd.substring(0, 2) + "," + cmd.substring(3, 5) + "): ");
    } else {    // could use implied AT, or check for startswith first, then default to specials
      //results = cmd + ": ";
      obd->command(cmd, cmd + ": ");
    }
  } else if (type == "cmdList") {
    // run multiple commands and return the results (as if they were entered separately - like init()
  } else {
    wsSend("{\"log\":\"server rcvd unexpected type:" + type + "\"}");
  }

}

void wsSend(String data) {
  DEBUGPRT.println("WS send: ");
  DEBUGPRT.println(data);
  ws->send(data);
}

// testing - easily can return with ws too
void getRawLog() {
  web->server->send(200, "text/plain", obd->log());
  obd->logClear();
}

void setup() {
  File f;
  int i;

  DEBUGPRT.begin(115200);
  DEBUGPRT.println("");
  DEBUGPRT.setDebugOutput(DEBUGLIB);

  if (!SPIFFS.begin()) {
    logFile = (File)NULL;
  } else {
    DEBUGPRT.println("Directory: /");
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      DEBUGPRT.print(dir.fileName());
      f = dir.openFile("r");
      DEBUGPRT.print(": ");
      DEBUGPRT.println(f.size());
    }
    logFile = SPIFFS.open(logFileName, "a");
  }
  
  WiFi.hostname(hhostname.c_str());
  WiFi.begin(ssid.c_str(), passphrase.c_str());

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUGPRT.print(".");
  }
  DEBUGPRT.println("");
  DEBUGPRT.print("Connected to ");
  DEBUGPRT.println(ssid);
  DEBUGPRT.print("IP address: ");
  DEBUGPRT.println(WiFi.localIP());

  if (mdns.begin(hhostname.c_str(), WiFi.localIP())) {
    DEBUGPRT.println("MDNS responder started");
  }

  web = new Web();
  web->addStatic("/", "/index.html");
  web->addStatic("/", "/");
  web->addRoute("/rawlog", getRawLog);
  web->addUpload("/upload");

  DEBUGPRT.println("http server started.");

  ws = new WebsocketServer(wsPort);
  ws->setConnect(wsConnect);
  ws->setDisconnect(wsDisconnect);
  ws->setReceive(wsReceive);
  ws->begin();
  DEBUGPRT.printf("ws server started on port %i.", wsPort);

  obd = new OBD(OBD_SERIAL);
  obd->logging = true;

  return;

  // don't do automatically anymore
  /*
  delay(500);
  DEBUGPRT.println("Initializing...");
  if (obd->init(dev)) {
    DEBUGPRT.println("Init OK! Device=");
    DEBUGPRT.println(obd->device);
    results += "Init OK! Device=" + obd->device;
  } else {
    DEBUGPRT.println("Init failed!");
    DEBUGPRT.println(obd->result);
    DEBUGPRT.println("***");
    results += "Init failed! " + obd->result;
  }
  //DEBUGPRT.println("...looping on ati...");
  */

}

void loop() {

  delay(10);
  web->server->handleClient();
  ws->process();

  if (ws->connected) {
    /*
    if (millis() > lastCommand + commandInterval) {
      DEBUGPRT.println("Sending command...");
      results += "ati: ";
      obd->command("ati");
      //results += "doing rpm()...\n";
      //int val = obd->rpm();
      //results += "rc=";
      //results += val;
      //results += " /" + obd->result + "/\n";
      lastCommand = millis();
      digitalWrite(statusPin, HIGH);
    }
    */
  }
  
  // with command queuing, 
  //  sending 25 'ati' in loop took 21s to get replies when no aggregating responses below at 1s interval.
  //  sending 25 'ati' in loop took 7s, 100 took about 27.5, so that seems to be the limit 
  // to aggregate results, need a reliable way to determine there's a result - can't use result string
  //  if it's prepended with stuff like command name, etc. when the command is sent; added obd.resultPending as
  //  handshake flag
  // prepending the result with command/etc here for logging doesn't work when queuing - that needs to be done
  //  when the command is popped by obd - so to do that, will have to send pre-built result string along with 
  //  command and queue that also; or do it automatically in obd, or add second type of command call to do it

  if (obd->process()) {
    if (obd->timeout) {
      DEBUGPRT.println("*** TIMEOUT!");
      DEBUGPRT.println(obd->result);
      DEBUGPRT.println("***");
      results += "TIMEOUT! " + obd->result;
    } else {
      DEBUGPRT.println(obd->result);
      DEBUGPRT.println("***");
      results += obd->result;
    }
    digitalWrite(statusPin, LOW);
    if (millis() > lastSend + sendInterval) {  // aggregate client updates - should use a vector of results to return array
      // should always be true here...
      if (obd->resultPending) {
        DEBUGPRT.println("Sending data...");
        wsSend("{\"log\":\"" + results + "\"}"); // results is string for now - IT SHOULD BE FIXED UP TO BE ACCEPTABLE FOR JSON!!!! (for error cases)
        obd->resultPending = false;
        results = "";
        lastSend = millis();
      }
    }
  }
  // if aggregating, there can be leftover results when everything's been done...
  if (millis() > lastSend + sendInterval) {  // aggregate client updates - should use a vector of results to return array
    if (obd->resultPending) {
      DEBUGPRT.println("Sending data...");
      wsSend("{\"log\":\"" + results + "\"}"); // results is string for now - IT SHOULD BE FIXED UP TO BE ACCEPTABLE FOR JSON!!!! (for error cases)
      obd->resultPending = false;
      results = "";
      lastSend = millis();
    }
  }

}
