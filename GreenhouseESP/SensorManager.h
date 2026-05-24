#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#if ARDUINO
#include <Arduino.h>
#include <DHT.h>
#else
#include "MockLibraries/Arduino.h"
#include "MockLibraries/DHT.h"
#endif

// Definizione dei Pin (Puoi cambiarli qui e cambieranno ovunque)
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