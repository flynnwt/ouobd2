# ESP8266 OBD2 Interface
![alt tag](https://raw.githubusercontent.com/flynnwt/ouobd2/master/ouobd2-capture.png)

## Background Info
This project is an implementation of an ESP8266 interface to a generic OBD2 device
(https://hackaday.io/project/8438-ouobd2-esp8266obd-ii). 

At this point, the code is mostly unfinished.  It is able to talk to the OBD2 device and
serves a webpage with websocket communication.

## Goals

1. Interface to OBD2 device
1. Provide 2-way websocket communication to read/write device
1. Keep custom configurations for vehicles, etc.

## Dependencies

* ESP8266 Arduino library

* Other libraries

  * ArduinoJson (https://github.com/bblanchon/ArduinoJson)
  
## Hardware Programming and Operation

0. use Arduino IDE or VSMicro to compile and upload code; my setup:
  * /libraries directories in ../Arduino/libraries directory
  * /ouobd2 files in a main project (../ouobd2) directory
 
0. use Arduino IDE with SPIFFS data tool to upload ../ouobd2/data  

0. serial console output (Serial1) for debug/monitoring

### Testing


## Examples

