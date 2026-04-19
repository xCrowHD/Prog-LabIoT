#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

#define DHTPIN D7
#define DHTTYPE DHT11
#define PHOTORESISTOR A0

// Struct per contenere tutti i dati della serra
struct PlantData {
  float temperature;
  float humidity;
  int light;
  bool valid;
};

class SensorManager {
private:
  DHT _dht;

public:
  SensorManager();
  void begin();
  PlantData getAllData();  // Legge tutto in un colpo solo
};

#endif