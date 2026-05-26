#ifndef HANDLE_EXCEPTIONS_H
#define HANDLE_EXCEPTIONS_H

#include "Interfaces/ILCDHandler.h"
#include "Interfaces/IInfluxHandler.h"
#include "Interfaces/IMqttHandler.h"
#include "Interfaces/ISensorManager.h"
#include "Interfaces/IAlarmHandler.h"

class HandleExceptions {
private:
    IAlarmHandler& _alarm;
    ILCDHandler &_lcd;
    IInfluxHandler &_client_idb;

public:
    HandleExceptions(IAlarmHandler& alarm, ILCDHandler &lcd, IInfluxHandler &client_idb);
    bool handleMqttExceptions(Thresholds &currentThr);
    bool handleThresholds(PlantData &data, Thresholds &currentThr, bool isDataValid);
    bool handleInfluxException(InfluxStatus status);
    bool handleDataException(PlantData &data);
    bool handleConnectionException(long rssi, const long RSSI_THRESHOLD);
    void handleSuccess();
};

#endif // HANDLE_EXCEPTIONS_H