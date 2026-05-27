#ifndef HANDLE_EXCEPTIONS_H
#define HANDLE_EXCEPTIONS_H

#include "LCDHandler.h"
#include "InfluxHandler.h"
#include "MqttHandler.h"
#include "SensorManager.h"
#include "AlarmHandler.h"

class HandleExceptions {
private:
    AlarmHandler _alarm;
    LCDHandler _lcd;
    InfluxHandler _client_idb;

public:
    HandleExceptions(AlarmHandler alarm, LCDHandler lcd, InfluxHandler client_idb);
    bool handleMqttExceptions(Thresholds &currentThr);
    bool handleThresholds(PlantData &data, Thresholds &currentThr);
    bool handleInfluxException(InfluxStatus status);
    bool handleDataException(PlantData &data);
    bool handleConnectionException(long rssi, const long RSSI_THRESHOLD);
    void handleSuccess();
};

#endif // HANDLE_EXCEPTIONS_H