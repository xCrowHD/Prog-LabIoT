#include "HandleExceptions.h"
#if ARDUINO
#include <Arduino.h>
#else
#include "MockLibraries/Serial.h" // <--- Inserisci il percorso corretto per raggiungere il tuo mock
#endif

HandleExceptions::HandleExceptions(AlarmHandler& alarm, LCDHandler& lcd, InfluxHandler& client_idb)
    : _alarm(alarm), _lcd(lcd), _client_idb(client_idb) {}

bool HandleExceptions::handleMqttExceptions(Thresholds &currentThr)
{
    if (currentThr.plantName == nullptr || currentThr.plantName[0] == '\0')
    {
        _alarm.addAlarm(AlarmType::NO_SEND_DATA);
        _lcd.addMessage("Error", "Missing plant!");
        Serial.println("MQTT exception: Missing plant name");
        return false;
    }
    _alarm.removeAlarm(AlarmType::NO_SEND_DATA);
    return true;
}

bool HandleExceptions::handleThresholds(PlantData &data, Thresholds &currentThr)
{
    if (!data.valid){
        _alarm.addAlarm(AlarmType::SENSOR_ERROR);
        _alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        _alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
        return false;
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
        return false;
    }
    else if (!tempInRange || !humInRange || !luxInRange)
    {
        _alarm.addAlarm(AlarmType::SOME_THRESHOLDS_OUT);
        _alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
        _lcd.addMessage("Thresholds", "SOME O.O.R");
        Serial.println(F("Some thresholds out of range!"));
        return false;
    }

    _alarm.removeAlarm(AlarmType::SOME_THRESHOLDS_OUT);
    _alarm.removeAlarm(AlarmType::ALL_THRESHOLDS_OUT);
    return true;
}

bool HandleExceptions::handleInfluxException(InfluxStatus status)
{
    if (status == InfluxStatus::ERR_INFLUX_CONNECTION)
    {
        _alarm.addAlarm(AlarmType::INFLUX_ERROR);
        Serial.print(F("Connection Error: "));
        Serial.println(_client_idb.getLastErrorMessage());
        return false;
    }

    _alarm.removeAlarm(AlarmType::INFLUX_ERROR);
    return true;
}

bool HandleExceptions::handleDataException(PlantData &data)
{

    if (!data.valid)
    {
        _alarm.addAlarm(AlarmType::SENSOR_ERROR);
        _lcd.addMessage("Error", "Sensor Error");
        Serial.println(F("Sensor Error!"));
        return false;
    }
    _alarm.removeAlarm(AlarmType::SENSOR_ERROR);
    return true;
}

bool HandleExceptions::handleConnectionException(long rssi, const long RSSI_THRESHOLD)
{
    if (rssi < RSSI_THRESHOLD)
    {
        _alarm.addAlarm(AlarmType::CONNECTION_ERROR);
        _lcd.addMessage("Error", "Connection Error");
        Serial.print(F("RSSI too low: "));
        Serial.println(rssi);
        return false;
    }
    return true;
}

void HandleExceptions::handleSuccess()
{
    _alarm.clearAlarms();
    _lcd.addMessage("All OK!");
    Serial.println(F("All OK!"));
}
