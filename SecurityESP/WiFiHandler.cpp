#include "WiFiHandler.h"

WiFiManager WiFiHandler::_wifiManager;

void WiFiHandler::begin() {
  _wifiManager.autoConnect();
}

void WiFiHandler::resetCredentials() {
  _wifiManager.resetSettings();
}

long WiFiHandler::getRSSI() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }

  return WiFi.RSSI();
}

//usiamo il MAC e non il chip ID perchè se gli ESP sono nello stesso batch hanno chip id uguale
void WiFiHandler::getMacAddress(char* buffer_size_13) {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  sprintf(buffer_size_13, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(buffer_size_13);
}