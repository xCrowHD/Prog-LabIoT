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
  PubSubClient _client;
  const char* _broker;
  int _port;
  Thresholds _plantThresholds;
  McuSettings _settings;
  bool _isStartMode = false;
  char _id[13];

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
  void sendImOn();
  bool isAddressedToMe(const JsonVariant& doc);
  void handleThresholds(byte* payload, unsigned int length);
  void handleStartStop(byte* payload, unsigned int length);
  void handleSettings(byte* payload, unsigned int length);
};

#endif