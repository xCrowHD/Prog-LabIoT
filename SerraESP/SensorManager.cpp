#include "SensorManager.h"

// Inizializzazione del sensore DHT nel costruttore
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

  // Controllo validità (isnan per DHT e controllo base per fotoresistenza)
  if (isnan(h) || isnan(t)) {
    Serial.println(F("ERRORE: Lettura DHT fallita!"));
    data.valid = false;
    return data;
  }

  data.temperatura = t;
  data.umidita = h;
  data.luce = (lightValue == 0) ? -1 : lightValue;

  if (data.luce == -1) {
    data.valid = false;
    return data;
  }

  data.valid = true;

  // Log di debug
  Serial.printf("\n--- Nuova Lettura ---\nTemp: %.2f C | Umid: %.2f %% | Luce: %d\n", t, h, data.luce);

  return data;
}