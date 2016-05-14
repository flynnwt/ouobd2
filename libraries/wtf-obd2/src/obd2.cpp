#include "obd2.h"

/*
 should init() be reset(), and init() then does atsp0, 01 00, etc. (set comm mode property for status)

 set ats0 to remove spaces? less traffic; easier to parse?
 set ath1 to receive headers? more complete info if needed

 useful builtins:
  monitor(on/off/filter) - use atma/atmr/atmp/atmt, etc. to stream info continuously; set a mode
    property? will want to return responses as they arrive

 HMMM...is elm putting \r\r between commands?? that would help deliminate them
*/

// put these in utilities.h???

unsigned long stringToHex(String s) {
  unsigned long val = 0;
  unsigned int i, v;
  char c;

  if (s.length() > 8) {
    return 0;
  }
  for (i = 0; i < s.length(); i++) {
    c = s.charAt(i);
    switch (c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      v = c - '0'; break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
      v = c - 'a' + 10; break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      v = c - 'A' + 10; break;
    default:
      return 0; break;
    }
    val = val | (v << (4 * (s.length() - i - 1)));
  }
  return val;
}

String stripCrLf(String s) {
  unsigned int i;
  String res = "";
  for (i = 0; i < s.length(); i++) {
    if ((s[i] != '\r') && (s[i] != '\n')) {
      res += s[i];
    }
  }
  return res;
}

String jsonify(String s) {
  unsigned int i;
  String res = "";

  for (i = 0; i < s.length(); i++) {
    switch (s[i]) {
    case '\"':
      res += "\\\"";
      break;
    case '\\':
      res += "\\\\";
      break;
      //case '/':
      //  res += "\\/";
      //  break;
    case '\b':
      res += "\\b";
      break;
    case '\f':
      res += "\\f";
      break;
    case '\n':
      res += "\\n";
      break;
    case '\r':
      res += "\\r";
      break;
    case '\t':
      res += "\\t";
      break;
    default:
      if ((s[i] < 32) || (s[i] > 126)) {
        char buf[5];
        sprintf(buf, "%04X", (int)s[i]);
        res += "\\u" + String(buf);
      } else {
        res += s[i];
      }
      break;
    }
  }
  return res;
}

OBD::OBD(HardwareSerial& p) : port(p) { // some weird way this has to be done
  char c;

  port.begin(DEFAULT_BAUD);
  rawLog = "";
  while (port.available()) {
    c = port.read();
    rawLog += char(c);;   // add to log; mode not known yet - user can clear ; ADD VERSION WITH logging arg!!!!!!!!!!!!!!!!!!!!!!
  }
  recvTimeout = DEFAULT_RECV_TIMEOUT;
  searchingTimeout = DEFAULT_SEARCHING_TIMEOUT;
  commandStart = 0;
  resultPending = false;
  monitoring = false;
  logging = false;
}

// atz resets all settings, BUT it responds based on the current settings (E0/E1)
bool OBD::init(String dev) {
  String saveResult;

  activeTimeout = DEFAULT_INIT_TIMEOUT;

  // on esp, the first command is failing with '?' (tried atz/ate0/ati); there wasn't
  //  anything leftover on the rx bus either; so just send anything safe and ignore result...
  //  maybe there is something left on tx for some reason - not sure but port.flush() didn't seem
  //  to show that there was
  // i dont think im seeing this when manually entering the command from client; so maybe it's a timing thing
  //  at start
  send("");
  while (!process()) {
    delay(10);
  };

  send("atz");
  while (!process()) {
    delay(10);
  };
  if (timeout) {
    result += "/ATZ TIMEOUT/";
    return false;
  } else if ((dev != "") && (result != dev) && (result != "atz" + dev)) {
    result += "/ATZ MISCOMPARE/" + result + "/" + dev + "/";
    if (jsonSafe) {
      result = jsonify(result);
    }
    return false;
  } else if (result.startsWith("atz")) {
    device = result.substring(3);
  } else {
    device = result;
  }

  saveResult = result;
  delay(100);
  send("ate0");
  while (!process()) {
    delay(10);
  };
  if (timeout) {
    result = saveResult + "/ATE0 TIMEOUT/";
    if (jsonSafe) {
      result = jsonify(result);
    }
    return false;
  } else if (result != "ate0OK") {
    result = saveResult + "/ATE0 MISCOMPARE/>";
    if (jsonSafe) {
      result = jsonify(result);
    }
    return false;
  }

  activeTimeout = recvTimeout;

  return true;
}

// command() and process() allow queuing when a new command is received
//  before the previous one is complete; autoDequeue enables automatic
//  sending of next command
//
// command() sends queued if available
// command(c) sends or queues c
// command(c,r) sends or queues c and pre-built result string; this allows caller to 
//   create a formatted output (for display), or id the commands, etc.  allow a template-type
//   replace?  %r% -> response
void OBD::command() {
  String c, r;

  if (commandStart == 0) {
    if (commandQueue.size() > 0) {
      c = commandQueue[0];
      commandQueue.pop_front();
      r = resultQueue[0];
      resultQueue.pop_front();
      command(c, r);
    }
  }
}

void OBD::command(String cmd) {
  command(cmd, "");
}

void OBD::command(String cmd, String result) {
  if (commandStart > 0) {
    commandQueue.push_back(cmd);
    resultQueue.push_back(result);
  } else {
    activeTimeout = recvTimeout;
    send(cmd, result);
    recv();
  }
}

unsigned int OBD::queued() {
  return commandQueue.size();
}

bool OBD::process() {
  if (commandStart > 0) {   // pending command
    if (millis() > commandStart + activeTimeout) {
      timeout = true;
      commandStart = 0;
      resultPending = true;
      return true;
    } else {                // check it
      recv();
      if (commandStart == 0) {
        return true;
      } else {
        return false;
      }
    }
  } else {                  // no pending command; send queued if one
    if (monitoring) {
      commandStart = millis();    // automatically start again to enable next recv() end indication
      recv();
    } else if (autoDequeue) {
      command();
    }
    return false;
  }
}

// set elm to monitor mode and return results continously; it is ended by any new send to elm
void OBD::monitor(bool mode) {
  // should probably play with timeout here too?? or just let them happen if they want to??
  if (mode) {
    crReplace = "<cr>";
    monitoring = true;
    send("atma");
  } else {
    crReplace = ""; // this is going to potentially affect the end piece of monitoring - need a cleaner way to end 
    // pause sends after this one for long enough to gracefully end monitoring (need new state for process() to wait for flush complete
    monitoring = false;
    send("");
  }

}

// PID-specific; gets the first 32 supported PIDs (others are requested at different addresses; should automatically do these;
//  would need to return vector instead - only works on ESP)
unsigned long OBD::supportedPIDs(String mode) {
  String val;
  unsigned long hexVal;

  send("01 " + mode);
  while (!process()) {
    delay(10);
  };
  if (timeout) {
    return 0;
  }
  result = "41 00 BE 3E B8 14";
  // 41 00 aa bb cc dd
  if (result.length() != 18 || (result.substring(0, 6) != "41 00 ")) {
    return 0;
  }
  val = result.substring(6, 8) + result.substring(9, 11) + result.substring(12, 14) + result.substring(15);
  hexVal = stringToHex(val);
  return hexVal;
}

// private helpers
void OBD::send(String cmd) {
  send(cmd, "");
}

void OBD::send(String cmd, String res) {
  int c;

  timeout = false;
  searching = false;
  result = res;
  if (port.available()) {
    result = "/SEND: RECV BUFFER !EMPTY/";
    while (port.available()) {
      c = port.read();
      result += char(c);
      if (logging) {
        rawLog += char(c);
      }
    }
    result += "/";
    if (jsonSafe) {
      result = jsonify(result);
    }
  }
  commandStart = millis() + 1;  // for arduino testing...it's actually still 0 when doing init()!
  if (logging) {
    rawLog += cmd + "\r";
  }
  if (rawLog.length() > maxLog) {
    rawLog = "";
  }
  port.print(cmd + "\r");
  port.flush();
}

void OBD::phonySend(String res) {
  timeout = false;
  searching = false;
  result = res;
}

// these will be very similar - what common functions???
// send, wait, check, parse, massage
int OBD::rpm() {
  int res = 0;

  //send("01 0C");
  //while (!process()) {
  //  delay(10);
  //};
  phonySend("41 0C 1A F8");
  if (timeout) {
    return -1;
  } else if (!result.startsWith("41 0C ")) {
    return -2;
  } else {
    res = stringToHex(result.substring(6, 8) + result.substring(9, 11));
    res = res >> 2;
    return res;
  }

}

void OBD::recv() {
  char c;

  while (port.available()) {
    c = port.read();
    if (logging && c) {     // ignore null
      rawLog += char(c);
    }
    if (c == '>') {  // we done (can send next command)
      commandStart = 0;
      resultPending = true;
      // shouldnt need this here since result should be safe unless it has cr or lf in it
      //if (jsonSafe) {
      //  result = jsonify(result);
      //}
      searching = false;
    } else if (c == '\n') {
      result += lfReplace;    // in case want to xlate or remove it
    } else if (c == 0) {
      // ignore NULL
    } else if (c == '\r') {
      result += crReplace;    // in case want to xlate or remove it; is needed for obd multilines
    } else {
      result += String(c);
    }
    if (result.endsWith("SEARCHING...")) { 
      result = result.substring(0, result.length() - 12);
      searching = true;
      activeTimeout = searchingTimeout;
    }
  }

  if (rawLog.length() > maxLog) {
    rawLog = "";
  }

}

String OBD::log() {
  return rawLog;
}

String OBD::logJson() {
  return jsonify(rawLog);
}

void OBD::log(bool mode) {
  logging = mode;
}

unsigned int OBD::logSize() {
  return rawLog.length();
}

void OBD::logClear() {
  rawLog = "";
}