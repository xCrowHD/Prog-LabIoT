#ifndef INFLUX_HANDLER_H
#define INFLUX_HANDLER_H

#if ARDUINO
#include <InfluxDbClient.h>
#include <Ticker.h>
#else
#include "MockLibraries/InfluxDbClient.h"
#include "MockLibraries/Ticker.h"
#endif

#include "SensorManager.h"
#include "MqttHandler.h"

enum class InfluxStatus
{
  SUCCESS,
  ERR_INFLUX_CONNECTION
};

class InfluxHandler{
  private:
    InfluxDBClient _clientIdb;
    long _rssiThreshold;
    Ticker _writeTicker;            
    volatile bool _flagWrite;

  public:
    InfluxHandler(const char* url, const char* org, const char* bucket, const char* token); 
    void begin(float defaultIntervalSeconds);
    bool isReadyToWrite();
    bool updateInterval(float newIntervalSeconds);
    InfluxStatus influxStatus(PlantData& data, const Thresholds& currentThr);
    InfluxStatus sendDataToInflux(PlantData& data, long rssi, const char* point_name, const char* device_name, const Thresholds& currentThr);
    const char* getLastErrorMessage();
};

#endif