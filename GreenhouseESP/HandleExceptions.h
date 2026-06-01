#ifndef HANDLE_EXCEPTIONS_H
#define HANDLE_EXCEPTIONS_H

#include "LCDHandler.h"
#include "InfluxHandler.h"
#include "MqttHandler.h"
#include "SensorManager.h"
#include "AlarmHandler.h"

struct HandleExceptionsConfig {
    bool testMode = false; // Se true, stampa i messaggi di debug per i LED anche su console
    float disableTime = 5.0f; // Durata in secondi della disabilitazione dell'allarme dopo un errore
    bool enable = true;
};

class HandleExceptions {
private:
    AlarmHandler& _alarm;
    LCDHandler& _lcd;
    InfluxHandler& _client_idb;
    HandleExceptionsConfig _config;
    uint32_t _internalTime;       // Tempo corrente (reale su Arduino, simulato su PC)
    uint32_t _deactivationTimestamp;
    bool _inBlindPeriod;

public:
    HandleExceptions(AlarmHandler& alarm, LCDHandler& lcd, InfluxHandler& client_idb, HandleExceptionsConfig config = HandleExceptionsConfig());
    void flipEnabled();
    bool handleMqttExceptions(Thresholds &currentThr);
    bool handleThresholds(PlantData &data, Thresholds &currentThr);
    bool handleInfluxException(InfluxStatus status);
    bool handleDataException(PlantData &data);
    bool handleConnectionException(long rssi, const long RSSI_THRESHOLD);
    void handleSuccess();
    bool isExecutionAllowed();

    HandleExceptionsConfig& getConfig();

    private:
     bool checkBlindPeriod();
    
};

#endif // HANDLE_EXCEPTIONS_H