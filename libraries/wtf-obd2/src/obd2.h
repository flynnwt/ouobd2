#ifndef _OBD2_h
#define _OBD2_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <deque>  // for command queueing

#define DEFAULT_BAUD 38400
#define DEFAULT_RECV_TIMEOUT 2000
#define DEFAULT_SEARCHING_TIMEOUT 10000
#define DEFAULT_INIT_TIMEOUT 5000

enum PID {
  RPM,
  EngineLoad,
  EngineLoadAbsolute,
  CoolantTemp,
  TimingAdvance,
  OilTemp,
  TorquePercentage,
  TorqueReference,
  IntakeTemp,
  IntakePressure,
  MAFPressure,
  BarometricPressure,
  Speed,
  Runtime,
  Distance,
  Throttle,
  AmbientTemp,
  ControlModuleVoltage
};

class OBD {
  HardwareSerial& port;
  unsigned long commandStart;
  unsigned long activeTimeout;
  bool searching;
  std::deque<String> commandQueue;
  std::deque<String> resultQueue;
  bool monitoring;
  String rawLog;

  void send(String cmd);
  void send(String cmd, String result);
  void recv();
  void phonySend(String result);

public:
  String device;
  unsigned long recvTimeout;
  unsigned long searchingTimeout;
  bool autoDequeue = true;
  bool timeout;
  bool resultPending;  // set here; reset by user if it needs to track pending results
  //String crReplace = "\r";
  String crReplace = "";  // i guess this has to default to strip; multiline commands can then set it to \r and deal with it
  String lfReplace = "\n";
  bool jsonSafe = true;
  String result;
  unsigned int maxLog = 1000;
  bool logging;

  OBD(HardwareSerial& port);
  bool init(String device);
  bool reset(String device);
  void command();
  void command(String cmd);
  void command(String cmd, String result);
  unsigned int queued();
  bool process();
  void monitor(bool mode);
  unsigned long supportedPIDs(String mode);
  int rpm();

  void log(bool mode);
  String log();
  String logJson();
  unsigned int logSize();
  void logClear();

};

#endif

