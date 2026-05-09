#include "MqttHandler.h"

MqttHandler::MqttHandler()
  : _client(512), _plantThresholds{}, _settings{} {

  WiFiHandler::getMacAddress(_id);
  snprintf(_lwtPayload, sizeof(_lwtPayload), "{\"id\":\"%s\",\"status\":\"OFFLINE\"}", _id);
}

void MqttHandler::begin(WiFiClient& wifiClient, const char* broker, int port) {
  _client.begin(broker, port, wifiClient);

  _client.onMessageAdvanced([this](MQTTClient* c, char t[], char p[], int l) {
    this->processMessage(c, t, p, l);
  });
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

    _client.setWill(TOPIC_CONNECTION, _lwtPayload, true, 1);

    if (_client.connect(_id)) {
      Serial.println(F("Connesso!"));
      _client.subscribe(TOPIC_THRESHOLD, 1);
      _client.subscribe(TOPIC_START_STOP, 1);
      _client.subscribe(TOPIC_MCU_SET, 1);
      sendImOn();
    } else {
      Serial.print(F("fallito, err="));
      Serial.print(_client.lastError());
      Serial.println(F(" riprovo tra 2 secondi"));
      delay(2000);  // Un po' di respiro
    }
  }
}

bool MqttHandler::connected() {
  return _client.connected();
}

void MqttHandler::processMessage(MQTTClient* client, char topic[], char payload[], int length) {
  if (strcmp(topic, TOPIC_THRESHOLD) == 0) {
    handleThresholds(payload, length);
  } else if (strcmp(topic, TOPIC_START_STOP) == 0) {
    handleStartStop(payload, length);
  } else if (strcmp(topic, TOPIC_MCU_SET) == 0) {
    handleSettings(payload, length);
  }
}

void MqttHandler::handleThresholds(char* payload, unsigned int length) {
  Serial.print(F("Payload ricevuto: "));
  Serial.write(payload, length);
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

void MqttHandler::handleStartStop(char* payload, unsigned int length) {
  Serial.print(F("Payload ricevuto: "));
  Serial.write(payload, length);
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

void MqttHandler::handleSettings(char* payload, unsigned int length) {
  Serial.print(F("Payload ricevuto: "));
  Serial.write(payload, length);
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
  if (strcmp(_id, doc["id"]) == 0) {
    return true;
  }
  return false;
}

void MqttHandler::sendImOn() {
  StaticJsonDocument<96> doc;
  char buffer[96];
  doc["id"] = _id;
  doc["status"] = "ONLINE";
  serializeJson(doc, buffer);

  char dynamicTopic[128];
  snprintf(dynamicTopic, sizeof(dynamicTopic), "%s/%s", TOPIC_CONNECTION, _id);

  _client.publish(dynamicTopic, buffer, true, 1);
}