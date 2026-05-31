#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#ifdef ARDUINO
  #include <Arduino.h>
  #include <DHT.h>
  #define DHTPIN D7
  #define DHTTYPE DHT11
  #define PHOTORESISTOR A0
#else
  #include "MockLibraries/DHT.h"
  #include "MockLibraries/Serial.h"
  #define DHTPIN 5
  #define DHTTYPE 111
  #define PHOTORESISTOR 6 
#endif



struct PlantData
{
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