#include "SensorManager.h"

SensorManager::SensorManager()
  : _dht(DHTPIN, DHTTYPE) {}

void SensorManager::begin() {
  _dht.begin();
  // Nota: Su ESP8266 A0 è tipicamente l'unico pin analogico e non serve pinMode
}

PlantData SensorManager::getAllData() {
  PlantData data;

  // 1. Lettura DHT
  float h = _dht.readHumidity();
  float t = _dht.readTemperature();

  // 2. Lettura Fotoresistenza
  int lightValue = analogRead(PHOTORESISTOR);

  if (isnan(h) || isnan(t)) {
    Serial.println(F("ERROR in DHT reading"));
    data.valid = false;
    return data;
  }

  data.temperature = t;
  data.humidity = h;
  data.light = lightValue;

  data.valid = true;

  // Log di debug
  Serial.printf("\n--- New Reaiding---\nTemp: %.2f C | Hum: %.2f %% | Light: %d\n", t, h, lightValue);

  return data;
}