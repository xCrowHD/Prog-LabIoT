#include "MqttHandler.h"

MqttHandler::MqttHandler()
  : _client(512), _settings{}, _topics{}, _status(Status::OFFLINE) {
  _actions[0] = "SECURITY";
  _actions[1] = "SETTINGS";
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
  if (strcmp(topic, _topics.settings) == 0) {
    handleSettings(payload, length);
  } else if (strcmp(topic, TOPIC_TOPICS) == 0) {
    handleTopics(payload, length);
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

      strlcpy(_settings.name, nameFromData, sizeof(_settings.name));
    }

    Serial.println(F("--- Settings Aggiornati ---"));
    Serial.print(F("Nome MCU: "));
    Serial.println(_settings.name);
  } else {
    Serial.println(F("Could no set settings"));
  }
}

void MqttHandler::handleTopics(char* payload, unsigned int length) {
  Serial.print(F("Payload ricevuto: "));
  Serial.write(payload, length);
  Serial.println();
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (!error) {

    if (!isAddressedToMe(doc)) {
      Serial.print(F("Msg Not for me"));
      Serial.println();
      return;
    }

    const char* sec = doc["security"];
    strlcpy(_topics.security, sec, sizeof(_topics.security));

    const char* set = doc["settings"];
    strlcpy(_topics.settings, set, sizeof(_topics.settings));

    _client.subscribe(_topics.security, 1);
    _client.subscribe(_topics.settings, 1);

    sendStatus(Status::ONLINE);
  } else {
    Serial.println(F("Could no set topics"));
  }
}

McuSettings MqttHandler::getSettings() {
  return _settings;
}


bool MqttHandler::isSet() {
  if (_settings.name[0] == '\0') {
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
  StaticJsonDocument<256> doc;
  char buffer[256];

  doc["id"] = _id;
  doc["status"] = statusToString(status);
  doc["type"] = "SECURITY_SENSOR";

  JsonArray actionsJson = doc["actions"].to<JsonArray>();
  for (int i = 0; i < NUM_ACTIONS; i++) {
    actionsJson.add(_actions[i]);
  }

  serializeJson(doc, buffer);

  _client.publish(_dynamicTopic, buffer, true, 1);
}

void MqttHandler::updateWill() {
  StaticJsonDocument<256> doc;

  doc["id"] = _id;
  doc["status"] = "OFFLINE";  // Il Will deve essere sempre OFFLINE
  doc["type"] = "SECURITY_SENSOR";
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

void MqttHandler::sendDoorPayload(bool doorClose) {
  StaticJsonDocument<256> doc;
  char buffer[256];

  time_t now = time(nullptr);

  doc["id"] = _id;
  doc["doorClose"] = doorClose;
  doc["timestamp"] = (unsigned long)now;
  doc["type"] = "DOOR_ALARM";
  serializeJson(doc, buffer);

  _client.publish(_topics.security, buffer, false, 1);
}

void MqttHandler::sendFlamePayload(bool isOnFlame, float temp) {
  StaticJsonDocument<256> doc;
  char buffer[256];

  time_t now = time(nullptr);

  doc["id"] = _id;
  doc["isOnFlame"] = isOnFlame;
  doc["temp"] = temp;
  doc["timestamp"] = (unsigned long)now;
  doc["type"] = "FLAME_ALARM";
  serializeJson(doc, buffer);

  _client.publish(_topics.security, buffer, false, 1);
}