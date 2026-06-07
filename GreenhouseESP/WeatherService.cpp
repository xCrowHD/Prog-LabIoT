#include "WeatherService.h"

WeatherService::WeatherService() {}

// Metodo privato per convertire il nome della città in coordinate
bool WeatherService::getCoordinates(const char* cityName) {
    if (WiFi.status() != WL_CONNECTED) return false;

    WiFiClient client;
    HTTPClient http;

    // Sostituiamo gli spazi con %20 per gli URL
    char safeCity[64] = "";
    urlEncodeSpace(cityName, safeCity, sizeof(safeCity));
    Serial.println(safeCity);

    char url[150];
    snprintf(url, sizeof(url), "http://geocoding-api.open-meteo.com/v1/search?name=%s&count=1", safeCity);

    http.begin(client, url);
    int httpCode = http.GET();
    bool success = false;

    if (httpCode == HTTP_CODE_OK) {
        StaticJsonDocument<512> doc; 
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error && doc["results"].size() > 0) {
            _latitude = doc["results"][0]["latitude"];
            _longitude = doc["results"][0]["longitude"];
            success = true;
        }
    }
    
    http.end();
    return success;
}

// Metodo pubblico principale chiamato dal tuo MqttHandler
void WeatherService::updateForecast(const char* cityName) {

    // 1. Recupera le coordinate della città
    if (!getCoordinates(cityName)) {
        Serial.println(F("[Weather] Impossibile recuperare le coordinate."));
        return;
    }

    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClient client;
    HTTPClient http;

    char url[180];
    snprintf(url, sizeof(url), 
             "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&daily=weather_code&forecast_days=1&timezone=auto", 
             _latitude, _longitude);

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, http.getStream());

        if (!error) {
            int weatherCode = doc["daily"]["weather_code"][0];

            // Logica dei codici WMO (61-65 pioggia, 80-82 rovesci, 95-99 temporali)
            if (weatherCode == 95 || info.weatherCode == 96 || info.weatherCode == 99) {
                Serial.println(F("[Weather] TEST."));
            } else if ((info.weatherCode >= 61 && info.weatherCode <= 65) || 
                       (info.weatherCode >= 80 && info.weatherCode <= 82)) {
                Serial.println(F("[Weather] TEST."));
            }
        }
    }

    http.end();
}

void WeatherService::urlEncodeSpace(const char* src, char* dest, size_t destLen) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < destLen - 1; i++) {
        if (src[i] == ' ') {
            if (j + 3 < destLen) {
                dest[j] = '%';
                j++;
                dest[j] = '2';
                j++;
                dest[j] = '0';
                j++;
            }
        } else {
            dest[j] = src[i];
            j++;
        }
    }
    dest[j] = '\0';
}