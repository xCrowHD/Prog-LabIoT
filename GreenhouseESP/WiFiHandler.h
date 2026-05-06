#ifndef WIFI_HANDLER_H
#define WIFI_HANDLER_H

#include <WiFiManager.h>
#include <ESP8266WiFi.h>


class WiFiHandler {
private:
  WiFiManager _wifiManager;
public:
  WiFiHandler();
  void begin();
  void resetCredentials();
  long getRSSI();
};

#endif