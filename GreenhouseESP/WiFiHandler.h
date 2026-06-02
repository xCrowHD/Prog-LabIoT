#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#ifdef ARDUINO
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#else 
#include "MockLibraries/WiFiManager.h"
#include "MockLibraries/ESP8266WiFi.h"
#include "MockLibraries/Serial.h"
#endif


class WiFiHandler {
private:
  static WiFiManager _wifiManager;
public:
  static void begin();
  static void resetCredentials();
  static long getRSSI();
  static void getMacAddress(char* buffer_size_13);
};

#endif