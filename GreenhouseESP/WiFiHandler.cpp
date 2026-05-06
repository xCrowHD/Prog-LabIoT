#include "WiFiHandler.h"

WiFiHandler::WiFiHandler() {}

void WiFiHandler::begin() {
  _wifiManager.autoConnect();
}

void WiFiHandler::resetCredentials() {
  _wifiManager.resetSettings();
}

long WiFiHandler::getRSSI(){
  while (WiFi.status() != WL_CONNECTED){
    delay(200);
  }

  return WiFi.RSSI();
}