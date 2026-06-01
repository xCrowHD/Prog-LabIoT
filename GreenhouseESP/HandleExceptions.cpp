#include "HandleExceptions.h"
#if ARDUINO
#include <Arduino.h>
#else
#include "MockLibraries/Serial.h" // <--- Inserisci il percorso corretto per raggiungere il tuo mock
#endif

#define ARE_ALARM_OFF() \
    if (checkBlindPeriod()) { \
        _alarm.addAlarm(AlarmType::NONE); \
        return true; \
    } else { \
        _alarm.removeAlarm(AlarmType::NONE); \
    }
    
#define ARE_ALARM_OFF_VOID() \
    if (checkBlindPeriod()) { \
        _alarm.addAlarm(AlarmType::NONE); \
        return; \
    } else { \
        _alarm.removeAlarm(AlarmType::NONE); \
    }


HandleExceptions::HandleExceptions(AlarmHandler& alarm, LCDHandler& lcd, InfluxHandler& client_idb, HandleExceptionsConfig config)
    : _alarm(alarm), _lcd(lcd), _client_idb(client_idb), _config{config} {}

void HandleExceptions::flipEnabled()
{
    unsigned long internalTime = millis();

    if (_config.enable)
    {
        _config.enable = false;
        _deactivationTimestamp = internalTime; // Registra il momento dello spegnimento
        _inBlindPeriod = true;      
        _alarm.getConfig().enable = false;     // Attiva la finestra di cecità totale
        _lcd.getConfig().enable = false;          // Disabilita la visualizzazione degli allarmi anche su LCD
        _alarm.clearAlarms();                  // Spegne anche i LED tramite AlarmHandler
    }
    else
    {
        _config.enable = true;
        _alarm.getConfig().enable = true;      // Riabilita il normale funzionamento dell'allarme
        _lcd.getConfig().enable = true;           // Riabilita la visualizzazione degli allarmi anche su LCD
        _inBlindPeriod = false;
    }
}

bool HandleExceptions::handleMqttExceptions(Thresholds &currentThr)
{
    ARE_ALARM_OFF();

    if (currentThr.plantName == nullptr || currentThr.plantName[0] == '\0')
    {
        _alarm.addAlarm(AlarmType::NO_SEND_DATA);
        _lcd.addMessage("Error", "Missing plant!");
        Serial.println("MQTT exception: Missing plant name");
        return !_config.enable;
    }
    _alarm.removeAlarm(AlarmType::NO_SEND_DATA);
    return true;
}

bool HandleExceptions::handleThresholds(PlantData &data, Thresholds &currentThr)
{
    ARE_ALARM_OFF();

    if (!data.valid){
        _alarm.addAlarm(AlarmType::SENSOR_ERROR);
        _alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        _alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
        _lcd.addMessage("Error", "Sensor Error");
        return !_config.enable;
    }
    bool tempInRange = data.temperature >= currentThr.tempMin && data.temperature <= currentThr.tempMax;
    bool humInRange = data.humidity >= currentThr.humMin && data.humidity <= currentThr.humMax;
    bool luxInRange = data.light >= currentThr.luxMin && data.light <= currentThr.luxMax;

    if (!tempInRange && !humInRange && !luxInRange)
    {
        _alarm.addAlarm(AlarmType::ALL_THRESHOLDS_OUT);
        _alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        _lcd.addMessage("Thresholds", "ALL O.O.R");
        Serial.println(F("All thresholds out of range"));
        return !_config.enable;
    }
    else if (!tempInRange || !humInRange || !luxInRange)
    {
        _alarm.addAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        _alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
        _lcd.addMessage("Thresholds", "SOME O.O.R");
        Serial.println(F("Some thresholds out of range!"));
        return !_config.enable;
    }

    _alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    _alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    return true;
}

bool HandleExceptions::handleInfluxException(InfluxStatus status)
{
    ARE_ALARM_OFF();

    if (status == InfluxStatus::ERR_INFLUX_CONNECTION)
    {
        _alarm.addAlarm(AlarmType::INFLUX_ERROR);
        Serial.print(F("Connection Error: "));
        Serial.println(_client_idb.getLastErrorMessage());
        return !_config.enable;
    }

    _alarm.removeAlarm(AlarmType::INFLUX_ERROR);
    return true;
}

bool HandleExceptions::handleDataException(PlantData &data)
{
    ARE_ALARM_OFF();

    if (!data.valid)
    {
        _alarm.addAlarm(AlarmType::SENSOR_ERROR);
        _lcd.addMessage("Error", "Sensor Error");
        Serial.println(F("Sensor Error!"));
        return !_config.enable;
    }
    _alarm.removeAlarm(AlarmType::SENSOR_ERROR);
    return true;
}

bool HandleExceptions::handleConnectionException(long rssi, const long RSSI_THRESHOLD)
{
    ARE_ALARM_OFF();

    if (rssi < RSSI_THRESHOLD)
    {
        _alarm.addAlarm(AlarmType::CONNECTION_ERROR);
        _lcd.addMessage("Error", "Connection Error");
        Serial.print(F("RSSI too low: "));
        Serial.println(rssi);
        return !_config.enable;
    }
    return true;
}

void HandleExceptions::handleSuccess()
{
    ARE_ALARM_OFF_VOID();

    _alarm.clearAlarms();
    _lcd.addMessage("All OK!");
    Serial.println(F("All OK!"));
}

HandleExceptionsConfig& HandleExceptions::getConfig() {
    return _config;
}

bool HandleExceptions::isExecutionAllowed()
{
    // Aggiorna lo stato del blind period se è scaduto a runtime
    if (_inBlindPeriod && (millis() - _deactivationTimestamp >= _config.disableTime))
    {
        _inBlindPeriod = false;
    }

    // I LED devono mostrare gli allarmi SOLO se il sistema è effettivamente abilitato
    // Se è disabilitato (anche se il blind period è scaduto), ritorniamo false -> LED spenti o ALL_OK
    return _config.enable;
}

bool HandleExceptions::checkBlindPeriod()
{
    if (_inBlindPeriod)
    {
        if (millis() - _deactivationTimestamp < _config.disableTime)
        {
            return true; // Siamo ancora nel blind period, ignora tutto
        }
        else
        {
            _inBlindPeriod = false; // Blind period scaduto
        }
    }
    return false;
}