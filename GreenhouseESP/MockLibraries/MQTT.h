#ifndef MOCK_MQTT_H
#define MOCK_MQTT_H

#include "ESP8266WiFi.h"
#include <iostream> 
#include <functional>

class MQTTClient {
    public:
        MQTTClient() {}
        MQTTClient(int bufSize) { (void)bufSize; }

        void begin(const char *broker, int port, WiFiClient &wifiClient) {  
            (void)broker;
            (void)port;
            (void)wifiClient;
        }
        bool connect() { return true; }
        bool connect(const char *id)
        {
            (void)id;
            return true;
        }
        bool connect(const char *id, const char *user, const char *pass)
        {
            (void)id;
            (void)user;
            (void)pass;
            return true;
        }
        bool connected() { return true; }

        void setWill(const char topic[], const char payload[], bool retained, int qos) {
            std::cout << "Last will set to" << payload << std::endl;
        }
        void clearWill()
        {
            std::cout << "Last will clear!" << std::endl;
        }
        void subscribe(const char *topic, int qos = 0)
        {
            (void)topic;
            (void)qos;
        }

        bool publish(const char *topic, const char *payload, bool retained = false, int qos = 0)
        {
            (void)topic;
            (void)payload;
            (void)retained;
            (void)qos;
            return true;
        }

        void onMessageAdvanced(std::function<void(MQTTClient *client, 
            char topic[], char bytes[], int length)>) {};

        
        void loop() {}

        const char * lastError() {
            return "Some MQTT error!";
        }
};

#endif // MOCK_MQTT_H