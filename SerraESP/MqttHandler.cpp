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