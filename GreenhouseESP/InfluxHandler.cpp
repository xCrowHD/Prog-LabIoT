#include "InfluxHandler.h"

InfluxHandler::InfluxHandler(const char* url, const char* org, const char* bucket, const char* token)
: _clientIdb(url, org, bucket, token), _flagWrite(false){
}

InfluxStatus InfluxHandler::influxStatus(PlantData& data, const Thresholds& currentThr){
  
  if (!_clientIdb.validateConnection()) { 
    return InfluxStatus::ERR_INFLUX_CONNECTION;
  }
    
  return InfluxStatus::SUCCESS;
}

InfluxStatus InfluxHandler::sendDataToInflux(PlantData& data, long rssi, const char* point_name, const char* device_name, const Thresholds& currentThr) {

  InfluxStatus status = influxStatus(data, currentThr);
  if (status == InfluxStatus::ERR_INFLUX_CONNECTION) {
        return InfluxStatus::ERR_INFLUX_CONNECTION;
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

  return InfluxStatus::SUCCESS;
}

const char* InfluxHandler::getLastErrorMessage()
{
  return _clientIdb.getLastErrorMessage().c_str();
}
