#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define TOPIC_THRESHOLD "lab_iot/mafogani/threshold"
#define TOPIC_START_STOP "lab_iot/mafogani/start-stop"

struct Thresholds {
  char platName[32];
  float tempMin;
  float tempMax;
  float humMin;
  float humMax;
  int luxMin;
  int luxMax;
};


class MqttHandler {
private:
  PubSubClient _client;
  const char* _broker;
  int _port;
  Thresholds _plantThresholds;
  bool _isStartMode = false;

  void handleThresholds(byte* payload, unsigned int length);
  void handleStartStop(byte* payload, unsigned int length);

public:
  MqttHandler(WiFiClient& wifiClient, const char* broker, int port);

  // Funzioni principali
  void begin(MQTT_CALLBACK_SIGNATURE);  // void begin(void (*callback)(char*, uint8_t*, unsigned int));
  void handle();
  void reconnect();
  bool connected();
  void processMessage(char* topic, byte* payload, unsigned int length);
  Thresholds getThresholds();
  bool isRunning();
  
};

#endif