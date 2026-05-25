#ifndef INFLUX_HANDLER_H
#define INFLUX_HANDLER_H

#include "interfaces/IInfluxHandler.h"

#include "SensorManager.h"
#include "MqttHandler.h"

#include <InfluxDbClient.h>
#include <Ticker.h>




class InfluxHandler : public IInfluxHandler {
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

    const char* getLastErrorMessage() override;
};


#endif