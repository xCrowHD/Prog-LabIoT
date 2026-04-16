#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define TOPIC_THRESHOLD "lab_iot/mafogani/threshold"
#define TOPIC_START_STOP "lab_iot/mafogani/start-stop"


class MqttHandler {
private:
  PubSubClient _client;
  const char* _broker;
  int _port;

public:
  // Costruttore
  MqttHandler(WiFiClient& wifiClient, const char* broker, int port);

  // Funzioni principali
  void begin(MQTT_CALLBACK_SIGNATURE);  // void begin(void (*callback)(char*, uint8_t*, unsigned int));
  void handle();
  void reconnect();
  bool connected();
};

#endif