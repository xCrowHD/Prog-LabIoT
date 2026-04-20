#include "MqttHandler.h"

MqttHandler::MqttHandler(WiFiClient& wifiClient, const char* broker, int port)
  : _client(wifiClient), _broker(broker), _port(port), _plantThresholds{} {}

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
    Serial.print(F("Tentativo connessione MQTT..."));
    if (_client.connect("ESP8266_Serra_Client")) {
      Serial.println(F("Connesso!"));
      _client.subscribe(TOPIC_THRESHOLD);
      _client.subscribe(TOPIC_START_STOP);
    } else {
      Serial.print(F("fallito, rc="));
      Serial.print(_client.state());
      Serial.println(F(" riprovo tra 2 secondi"));
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
  Serial.print(F("Payload ricevuto: "));
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error) {

    if (doc.containsKey("name")) {
      const char* nameFromData = doc["name"];

      // Copiamo il nome nel nostro array fisso (max 31 char + \0)
      strncpy(_plantThresholds.platName, nameFromData, sizeof(_plantThresholds.platName) - 1);

      // Assicuriamoci che la stringa sia chiusa correttamente
      _plantThresholds.platName[sizeof(_plantThresholds.platName) - 1] = '\0';
    }
    JsonObject thresholds = doc["thresholds"];
    Serial.println(F("Aggiornamento soglie!"));

    _plantThresholds.tempMin = thresholds["temp"]["min"];
    _plantThresholds.tempMax = thresholds["temp"]["max"];

    _plantThresholds.humMin = thresholds["hum"]["min"];
    _plantThresholds.humMax = thresholds["hum"]["max"];

    _plantThresholds.luxMin = thresholds["light"]["min"];
    _plantThresholds.luxMax = thresholds["light"]["max"];
    Serial.println(F("--- Dati Aggiornati ---"));
    Serial.print(F("Nuova Pianta: "));
    Serial.println(_plantThresholds.platName);
  } else {
    Serial.println(F("Could no set thresholds"));
  }
}

void MqttHandler::handleStartStop(byte* payload, unsigned int length) {
  Serial.print(F("Payload ricevuto: "));
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (length == 5 && memcmp(payload, "START", 5) == 0) {
    _isStartMode = true;
    Serial.println(F("Stato: ATTIVO"));
  } else if (length == 4 && memcmp(payload, "STOP", 4) == 0) {
    _isStartMode = false;
    Serial.println(F("Stato: DISATTIVATO"));
  } else {
    Serial.print(F("Comando sconosciuto di lunghezza: "));
    Serial.println(length);
  }
}

Thresholds MqttHandler::getThresholds() {
  return _plantThresholds;
}

bool MqttHandler::isRunning() {
  return _isStartMode;
}