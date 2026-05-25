#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include "Interfaces/IMqttHandler.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>
//usiamo staicJsonDocument cosi da mettere nello stack ed evita frammentazione
//se usassimo JsonDocument lo metterebbe nell'heap portando possibile frammentazione in seguito
#include "WiFiHandler.h"


#define TOPIC_CONNECTION "lab_iot/mafogani/connection"
#define TOPIC_TOPICS "lab_iot/mafogani/topics"
//#define TOPIC_THRESHOLD "lab_iot/mafogani/threshold"
//#define TOPIC_START_STOP "lab_iot/mafogani/start-stop"
//#define TOPIC_MCU_SET "lab_iot/mafogani/set-mcu"
//#define TOPIC_BACKUP "lab_iot/mafogani/backup"

class MqttHandler : public IMqttHandler{
  private:
    MQTTClient _client;
    char _lwtPayload[64];
    Thresholds _plantThresholds;
    McuSettings _settings;
    bool _isStartMode = false;
    bool _isStandBy = false;
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
    Thresholds getThresholds();
    bool isRunning();
    bool isSet();
    McuSettings getSettings();
    bool isStandBy();


  private:
    const char* statusToString(Status s);
    void processMessage(MQTTClient* client, char topic[], char payload[], int length);
    void sendStatus(Status status);
    bool isAddressedToMe(const JsonVariant& doc);
    void handleThresholds(char* payload, unsigned int length);
    void handleStartStop(char* payload, unsigned int length);
    void handleSettings(char* payload, unsigned int length);
    void handleStandBy(char* payload, unsigned int length);
    void handleTopics(char* payload, unsigned int length);
};

#endif