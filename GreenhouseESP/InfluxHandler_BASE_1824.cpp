#include "InfluxHandler.h"

#include <Ticker.h>
#include <InfluxDbClient.h>
#include "MqttHandler.h"
#include "SensorManager.h"

InfluxHandler::InfluxHandler(const char* url, const char* org, const char* bucket, const char* token)
: _clientIdb(url, org, bucket, token), _flagWrite(false){
}

void InfluxHandler::begin(float defaultIntervalSeconds) {
    updateInterval(defaultIntervalSeconds);
}

byte InfluxHandler::updateInterval(float newIntervalSeconds) { //aggiungere logica gestione errori
  // Protezione: evita valori assurdi o zero che farebbero crashare l'ESP
  if (newIntervalSeconds <= 0)
    return 1;

  Serial.print(F("Aggiornamento intervallo InfluxDB: "));
  Serial.print(newIntervalSeconds);
  Serial.println(F(" secondi"));

  // Stacchiamo il ticker e lo riattacchiamo con il nuovo valore
  _writeTicker.detach();
  _writeTicker.attach(newIntervalSeconds, [this]() {
    _flagWrite= true;
  });

  return 0;
}

bool InfluxHandler::isReadyToWrite() {
    if (_flagWrite) {
        _flagWrite = false; 
        return true;
    }
    return false;
}

InfluxStatus InfluxHandler::dataStatus(PlantData& data, const Thresholds& currentThr){

  if (currentThr.plantName == nullptr || currentThr.plantName[0] == '\0') {
    return InfluxStatus::ERR_INVALID_PLANT;
  }
  
  if (!_clientIdb.validateConnection()) { 
    return InfluxStatus::ERR_INFLUX_CONNECTION;
  }
  
  bool tempInRange = data.temperature >= currentThr.tempMin && data.temperature <= currentThr.tempMax;
  bool humInRange = data.humidity >= currentThr.humMin && data.humidity <= currentThr.humMax;
  bool luxInRange = data.light >= currentThr.luxMin && data.light <= currentThr.luxMax;

  if (!tempInRange && !humInRange && !luxInRange) {
    return InfluxStatus::WARN_THRESHOLDS_ALL;
  } 
    
  if (!tempInRange || !humInRange || !luxInRange) {
    return InfluxStatus::WARN_THRESHOLDS_SOME;            
  }
    
  return InfluxStatus::SUCCESS;
}

InfluxStatus InfluxHandler::sendDataToInflux(PlantData& data, long rssi, const char* point_name, const char* device_name, const Thresholds& currentThr) {

  InfluxStatus status = dataStatus(data, currentThr);
  if (status == InfluxStatus::ERR_INFLUX_CONNECTION || status == InfluxStatus::ERR_INVALID_PLANT) {
        return status;
    }

  Point sensorData(point_name);
  sensorData.addTag("device", device_name);
  sensorData.addTag("pianta", currentThr.plantName);
  sensorData.addField("temp", data.temperature);
  sensorData.addField("hum", data.humidity);
  sensorData.addField("lux", data.light);
  sensorData.addField("rssi", rssi);

  if(!_clientIdb.writePoint(sensorData)){
    return InfluxStatus::ERR_INFLUX_CONNECTION;
  }

  return status;
}

InfluxDBClient& InfluxHandler::getInfluxClient(){
  return _clientIdb;
}

