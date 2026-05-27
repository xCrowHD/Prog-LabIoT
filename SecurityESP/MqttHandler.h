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
#define TOPIC_TOPICS "lab_iot/mafogani/topics"

struct Topics {
  char security[32] = "";
  char settings[32] = "";
};

struct McuSettings {
  char name[32] = "";
};

enum class Status {
  ONLINE,
  CONNECTING,
  OFFLINE,
};

class MqttHandler {
private:
  MQTTClient _client;
  char _lwtPayload[64];
  McuSettings _settings;
  char _id[13];
  char _dynamicTopic[128];
  Topics _topics;
  Status _status = Status::OFFLINE;

public:
  // Costruttore
  MqttHandler();

  // Funzioni principali
  void begin(WiFiClient& wifiClient, const char* broker, int port);
  void handle();
  void reconnect();
  bool connected();
  McuSettings getSettings();
  bool isSet();
  void sendSecurityPayload();

private:
  const char* statusToString(Status s);
  void processMessage(MQTTClient* client, char topic[], char payload[], int length);
  void sendStatus(Status status);
  bool isAddressedToMe(const JsonVariant& doc);
  void handleTopics(char* payload, unsigned int length);
  void handleSettings(char* payload, unsigned int length);
};

#endif