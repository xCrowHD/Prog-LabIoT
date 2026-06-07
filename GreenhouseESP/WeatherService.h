#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

class WeatherService {
private:
    float _latitude = 0.0;
    float _longitude = 0.0;
    bool getCoordinates(const char* cityName);
    void urlEncodeSpace(const char* src, char* dest, size_t destLen);

public:
    WeatherService();
    void updateForecast(const char* cityName);
};

#endif