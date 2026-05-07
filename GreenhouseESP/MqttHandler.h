#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
//usiamo staicJsonDocument cosi da mettere nello stack ed evita frammentazione
//se usassimo JsonDocument lo metterebbe nell'heap portando possibile frammentazione in seguito
#include "WiFiHandler.h"

#define TOPIC_CONNECTION "lab_iot/mafogani/connection"
#define TOPIC_THRESHOLD "lab_iot/mafogani/threshold"
#define TOPIC_START_STOP "lab_iot/mafogani/start-stop"
#define TOPIC_MCU_SET "lab_iot/mafogani/set-mcu"

struct Thresholds {
  char platName[32];
  float tempMin;
  float tempMax;
  float humMin;
  float humMax;
  int luxMin;
  int luxMax;
};

struct McuSettings {
  char mcuName[32];
  bool isBackup;
  float timer;
};

class MqttHandler {
private:
  PubSubClient _client;
  const char* _broker;
  int _port;
  Thresholds _plantThresholds;
  McuSettings _settings;
  bool _isStartMode = false;

public:
  // Costruttore
  MqttHandler(WiFiClient& wifiClient, const char* broker, int port);

  // Funzioni principali
  void begin(MQTT_CALLBACK_SIGNATURE);  // void begin(void (*callback)(char*, uint8_t*, unsigned int));
  void handle();
  void reconnect();
  bool connected();
  void processMessage(char* topic, byte* payload, unsigned int length);
  Thresholds getThresholds();
  bool isRunning();
  bool isSet();
  McuSettings getSettings();


private:
  bool isAddressedToMe(const JsonVariant& doc);
  void handleThresholds(byte* payload, unsigned int length);
  void handleStartStop(byte* payload, unsigned int length);
  void handleSettings(byte* payload, unsigned int length);
};

#endif