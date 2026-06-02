#ifndef MOCK_ESP8266_WIFI_H
#define MOCK_ESP8266_WIFI_H

#include <iostream>
#include <cstdint> 

const int WL_CONNECTED = 3;

class WiFiClient
{
public:
    WiFiClient() {}

    int connect(const char *host, uint16_t port) { return 1; }
    void stop() {}
    bool connected() { return true; }
};

class ESP8266WiFiClass
{
public:
    ESP8266WiFiClass() {}

    void begin(const char *ssid, const char *passphrase)
    {
        std::cout << "WiFi: Connecting to " << ssid << "..." << std::endl;
    }

    int status()
    {
        return WL_CONNECTED;
    }

    long RSSI()
    {
        return -40; 
    }
    
    void macAddress(uint8_t *mac)
    {
        mac[0] = 0x00;
        mac[1] = 0x11;
        mac[2] = 0x22;
        mac[3] = 0xAA;
        mac[4] = 0xBB;
        mac[5] = 0xCC;
    }
};

inline ESP8266WiFiClass WiFi;

#endif // MOCK_ESP8266_WIFI_H