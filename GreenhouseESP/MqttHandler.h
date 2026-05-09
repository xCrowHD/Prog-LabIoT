#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>
//usiamo staicJsonDocument cosi da mettere nello stack ed evita frammentazione
//se usassimo JsonDocument lo metterebbe nell'heap portando possibile frammentazione in seguito
#include "WiFiHandler.h"

#define TOPIC_CONNECTION "lab_iot/mafogani/connection"
#define TOPIC_THRESHOLD "lab_iot/mafogani/threshold"
#define TOPIC_START_STOP "lab_iot/mafogani/start-stop"
#define TOPIC_MCU_SET "lab_iot/mafogani/set-mcu"

struct Thresholds {
  char platName[32] = "";
  float tempMin = 0;
  float tempMax = 0;
  float humMin = 0;
  float humMax = 0;
  int luxMin = 0;
  int luxMax = 0;
};

struct McuSettings {
  char mcuName[32] = "";
  bool isBackup = false;
  float timer = 0;
};

class MqttHandler {
private:
  MQTTClient _client;
  char _lwtPayload[64];
  Thresholds _plantThresholds;
  McuSettings _settings;
  bool _isStartMode = false;
  char _id[13];

public:
  // Costruttore
  MqttHandler();

  // Funzioni principali
  void begin(WiFiClient& wifiClient, const char* broker, int port);
  void handle();
  void reconnect();
  bool connected();
  Thresholds getThresholds();
  bool isRunning();
  bool isSet();
  McuSettings getSettings();


private:
  void processMessage(MQTTClient* client, char topic[], char payload[], int length);
  void sendImOn();
  bool isAddressedToMe(const JsonVariant& doc);
  void handleThresholds(char* payload, unsigned int length);
  void handleStartStop(char* payload, unsigned int length);
  void handleSettings(char* payload, unsigned int length);
};

#endif