#include "MqttHandler.h"

MqttHandler::MqttHandler(WiFiClient& wifiClient, const char* broker, int port)
  : _client(wifiClient), _broker(broker), _port(port) {}

void MqttHandler::begin(MQTT_CALLBACK_SIGNATURE) {
  _client.setServer(_broker, _port);
  _client.setCallback(callback);
}

void MqttHandler::handle() {
  if (!_client.connected()) {
    reconnect();
  }
  _client.loop();
}

void MqttHandler::reconnect() {
  while (!_client.connected()) {
    Serial.print("Tentativo connessione MQTT...");
    if (_client.connect("ESP8266_Serra_Client")) {
      Serial.println("Connesso!");
      _client.subscribe(TOPIC_THRESHOLD);
      _client.subscribe(TOPIC_START_STOP);
    } else {
      Serial.print("fallito, rc=");
      Serial.print(_client.state());
      Serial.println(" riprovo tra 2 secondi");
      delay(2000);  // Un po' di respiro
    }
  }
}

bool MqttHandler::connected() {
  return _client.connected();
}

void MqttHandler::processMessage(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, TOPIC_THRESHOLD) == 0) {
    handleThresholds(payload, length);
  } else if (strcmp(topic, TOPIC_START_STOP) == 0) {
    handleStartStop(payload, length);
  }
}

void MqttHandler::handleThresholds(byte* payload, unsigned int length) {
  Serial.print("Payload ricevuto: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  Serial.println("Aggiornamento soglie ricevuto!");
  // Logica di parsing...
}

void MqttHandler::handleStartStop(byte* payload, unsigned int length) {
  Serial.print("Payload ricevuto: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
}

Thresholds MqttHandler::getThresholds() {
  return _plantThresholds;
}

bool MqttHandler::isRunning() {
  return _isStartMode;
}