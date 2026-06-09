#include "MqttHandler.h"
#include "WeatherService.h"

MqttHandler::MqttHandler()
  : _client(512), _plantThresholds{}, _settings{}, _topics{}, _status(Status::OFFLINE) {
  _actions[0] = "SYNCPLANT";
  _actions[1] = "SETTINGS";
  _actions[2] = "STARTSTOP";
  _actions[3] = "BACKUP";
}

void MqttHandler::begin(WiFiClient& wifiClient, const char* broker, int port) {
  WiFiHandler::getMacAddress(_id);
  updateWill();

  _client.begin(broker, port, wifiClient);

  snprintf(_dynamicTopic, sizeof(_dynamicTopic), "%s/%s", TOPIC_CONNECTION, _id);
  _client.setWill(_dynamicTopic, _lwtPayload, true, 1);

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
    _client.clearWill();
    _client.setWill(_dynamicTopic, _lwtPayload, true, 1);
    if (_client.connect(_id)) {
      Serial.println(F("Connesso!"));
      _client.subscribe(TOPIC_TOPICS, 1);
      sendStatus(Status::CONNECTING);
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
  if (strcmp(topic, _topics.threshold) == 0) {
    handleThresholds(payload, length);
  } else if (strcmp(topic, _topics.running) == 0) {
    handleStartStop(payload, length);
  } else if (strcmp(topic, _topics.set) == 0) {
    handleSettings(payload, length);
  } else if (strcmp(topic, _topics.backup) == 0) {
    handleStandBy(payload, length);
  } else if (strcmp(topic, TOPIC_TOPICS) == 0) {
    handleTopics(payload, length);
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
      strlcpy(_plantThresholds.plantName, nameFromData, sizeof(_plantThresholds.plantName));
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
    Serial.println(_plantThresholds.plantName);
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

void MqttHandler::handleStandBy(char* payload, unsigned int length) {
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
    Serial.println(F("--- Standby Aggiornato ---"));
    _isStandBy = doc["standby"];
    Serial.println(_isStandBy ? F("Standby") : F("Not Standby"));
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

    if (doc.containsKey("backup")) {
      _settings.isBackup = doc["backup"];
    }

    if (doc.containsKey("timer")) {
      _settings.timer = doc["timer"];
    }

    if (doc.containsKey("location")) {

      const char* locationFromData = doc["location"];
      if (strcmp(_settings.location, locationFromData) != 0) {
        Serial.print(F("Nuova location rilevata: "));
        Serial.println(locationFromData);
        WeatherService weather;
        weather.updateForecast(locationFromData);
      }

      strlcpy(_settings.location, locationFromData, sizeof(_settings.location));
    }


    Serial.println(F("--- Settings Aggiornati ---"));
    Serial.print(F("Nome MCU: "));
    Serial.println(_settings.mcuName);
    Serial.print(F("Timer MCU: "));
    Serial.println(_settings.timer);
  } else {
    Serial.println(F("Could no set settings"));
  }
}

void MqttHandler::handleTopics(char* payload, unsigned int length) {
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

    const char* thr = doc["syncplant"];
    strlcpy(_topics.threshold, thr, sizeof(_topics.threshold));

    const char* run = doc["startstop"];
    strlcpy(_topics.running, run, sizeof(_topics.running));

    const char* set = doc["settings"];
    strlcpy(_topics.set, set, sizeof(_topics.set));

    const char* backup = doc["backup"];
    strlcpy(_topics.backup, backup, sizeof(_topics.backup));

    _client.subscribe(_topics.threshold, 1);
    _client.subscribe(_topics.running, 1);
    _client.subscribe(_topics.set, 1);
    _client.subscribe(_topics.backup, 1);

    sendStatus(Status::ONLINE);
  } else {
    Serial.println(F("Could no set topics"));
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

bool MqttHandler::isStandBy() {
  return _isStandBy;
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

void MqttHandler::sendStatus(Status status) {
  StaticJsonDocument<512> doc;
  char buffer[512];

  doc["id"] = _id;
  doc["status"] = statusToString(status);
  doc["type"] = "PLANT_SENSOR";

  JsonArray actionsJson = doc["actions"].to<JsonArray>();
  for (int i = 0; i < NUM_ACTIONS; i++) {
    actionsJson.add(_actions[i]);
  }

  serializeJson(doc, buffer);

  _client.publish(_dynamicTopic, buffer, true, 1);
}

void MqttHandler::updateWill() {
  StaticJsonDocument<512> doc;

  doc["id"] = _id;
  doc["status"] = "OFFLINE";  // Il Will deve essere sempre OFFLINE
  doc["type"] = "PLANT_SENSOR";
  // Aggiungiamo anche le actions se vuoi coerenza totale
  JsonArray actionsJson = doc["actions"].to<JsonArray>();
  for (int i = 0; i < NUM_ACTIONS; i++) {
    actionsJson.add(_actions[i]);
  }

  serializeJson(doc, _lwtPayload);
}

const char* MqttHandler::statusToString(Status s) {
  switch (s) {
    case Status::CONNECTING: return "CONNECTING";
    case Status::ONLINE: return "ONLINE";
    case Status::OFFLINE: return "OFFLINE";
    default: return "UNKNOWN";
  }
}