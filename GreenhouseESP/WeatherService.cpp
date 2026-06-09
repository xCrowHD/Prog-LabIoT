#include "WeatherService.h"
#include "secrets.h"

WeatherService::WeatherService()
  : _notifier(BOT_TOKEN, TELEGRAM_CHAT_ID), _latitude(0.0), _longitude(0.0) {
}

void WeatherService::begin() {
  _notifier.begin();
}

// Metodo privato per convertire il nome della città in coordinate
bool WeatherService::getCoordinates(const char* cityName) {
  Serial.print(F("[Weather] getCoordinates chiamata per: "));
  Serial.println(cityName);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("[Weather] ERRORE: Wi-Fi non connesso! Stato: "));
    Serial.println(WiFi.status());
    return false;
  }

  WiFiClient client;
  HTTPClient http;

  // Sostituiamo gli spazi con %20 per gli URL
  char safeCity[64] = "";
  urlEncodeSpace(cityName, safeCity, sizeof(safeCity));
  Serial.print(F("[Weather] Nome città codificato per URL: "));
  Serial.println(safeCity);

  char url[180];
  snprintf(url, sizeof(url), "http://geocoding-api.open-meteo.com/v1/search?name=%s&count=1", safeCity);
  Serial.print(F("[Weather] Richiesta Geocoding URL: "));
  Serial.println(url);

  http.begin(client, url);
  int httpCode = http.GET();
  Serial.print(F("[Weather] Geocoding HTTP Code ottenuto: "));
  Serial.println(httpCode);

  bool success = false;

  if (httpCode == HTTP_CODE_OK) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, http.getString());

    if (error) {
      Serial.print(F("[Weather] Errore parsing JSON Geocoding: "));
      Serial.println(error.f_str());
    } else {
      JsonArray results = doc["results"];

      if (results.size() > 0) {
        _latitude = results[0]["latitude"];
        _longitude = results[0]["longitude"];

        Serial.print(F("[Weather] Coordinate trovate! Lat: "));
        Serial.print(_latitude, 4);
        Serial.print(F(" | Lon: "));
        Serial.println(_longitude, 4);
        success = true;
      } else {
        Serial.println(F("[Weather] Nessun risultato trovato per questa città."));
      }
    }
  } else {
    Serial.print(F("[Weather] Richiesta Geocoding fallita, codice HTTP spec: "));
    Serial.println(httpCode);
  }

  http.end();
  return success;
}


void WeatherService::updateForecast(const char* cityName) {
  if (!getCoordinates(cityName)) {
    Serial.println(F("[Weather] Impossibile recuperare le coordinate."));
    return;
  }

  WiFiClient client;
  HTTPClient http;

  char url[180];
  snprintf(url, sizeof(url),
           "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&daily=weather_code&forecast_days=1&timezone=auto",
           _latitude, _longitude);

  http.begin(client, url);
  int httpCode = http.GET();

  // Creiamo una variabile locale per salvarci il codice ed usarlo DOPO
  int weatherCodeToSend = -1; 

  if (httpCode == HTTP_CODE_OK) {
    
    // Il blocco JSON vive SOLO dentro queste graffe (Scope limitato per liberare RAM)
    {
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, http.getString());
      if (!error) {
        weatherCodeToSend = doc["daily"]["weather_code"][0];
        Serial.print(F("[Weather] Today Code recuperato: "));
        Serial.println(weatherCodeToSend);
      }
    } // Qui l'oggetto 'doc' viene DISTRUTTO e la sua RAM viene liberata immediatamente!
  } else {
    Serial.println(F("[Weather] Errore richiesta forecast"));
  }

  http.end(); 

  if (weatherCodeToSend != -1) {
    Serial.println(F("[Weather] RAM liberata. Avvio notifica sicura su Telegram..."));
    _notifier.sendWeatherAlert(weatherCodeToSend, cityName);
  }

  Serial.println(F("--- [Weather] FINE AGGIORNAMENTO METEO ---\n"));
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