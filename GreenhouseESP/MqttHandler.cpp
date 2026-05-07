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
      _client.subscribe(TOPIC_MCU_SET);
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
  } else if (strcmp(topic, TOPIC_MCU_SET) == 0) {
    handleSettings(payload, length);
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

    if (!isAddressedToMe(doc)) {
      Serial.print(F("Msg Not for me"));
      Serial.println();
      return;
    }

    if (doc.containsKey("name")) {
      const char* nameFromData = doc["name"];
      strlcpy(_plantThresholds.platName, nameFromData, sizeof(_plantThresholds.platName));
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

  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error) {
    if (!isAddressedToMe(doc)) {
      Serial.print(F("Msg Not for me"));
      Serial.println();
      return;
    }

    _isStartMode = doc["status"];
    Serial.print(F("Stato aggiornato: "));
    Serial.println(_isStartMode ? F("ATTIVO") : F("DISATTIVATO"));
  }
}

void MqttHandler::handleSettings(byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error) {

    if (!isAddressedToMe(doc)) {
      Serial.print(F("Msg Not for me"));
      Serial.println();
      return;
    }

    if (doc.containsKey("name")) {
      const char* nameFromData = doc["name"];

      strlcpy(_settings.mcuName, nameFromData, sizeof(_settings.mcuName));
    }
    _settings.isBackup = doc["backup"];
    _settings.timer = doc["timer"];

    Serial.println(F("--- Settings Aggiornati ---"));
    Serial.print(F("Nome MCU: "));
    Serial.println(_settings.mcuName);
    Serial.print(F("Timer MCU: "));
    Serial.println(_settings.timer);
  } else {
    Serial.println(F("Could no set settings"));
  }
}

Thresholds MqttHandler::getThresholds() {
  return _plantThresholds;
}

McuSettings MqttHandler::getSettings() {
  return _settings;
}

bool MqttHandler::isRunning() {
  return _isStartMode;
}

bool MqttHandler::isSet() {
  if (_settings.mcuName[0] == '\0') {
    return false;
  }
  if (_settings.timer <= 0) {
    return false;
  }
  return true;
}

bool MqttHandler::isAddressedToMe(const JsonVariant& doc) {
  char myMac[13];
  WiFiHandler::getMacAddress(myMac);
  if (strcmp(myMac, doc["id"]) == 0) {
    return true;
  }
  return false;
}