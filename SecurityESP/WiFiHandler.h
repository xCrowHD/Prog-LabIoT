#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <WiFiManager.h>
#include <ESP8266WiFi.h>


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