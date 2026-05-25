#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "Interfaces/ISensorManager.h"
#include <Arduino.h>
#include <DHT.h>


// Definizione dei Pin (Puoi cambiarli qui e cambieranno ovunque)
#define DHTPIN D7
#define DHTTYPE DHT11
#define PHOTORESISTOR A0

class SensorManager {
private:
  DHT _dht;

public:
  SensorManager();
  void begin();
  PlantData getAllData();  // Legge tutto in un colpo solo
};

#endif