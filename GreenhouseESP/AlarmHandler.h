#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H
#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include "MockLibraries/Serial.h"
  #include <cstdint>
  #include <iostream>
#endif

#include "LCDHandler.h"
#include <set>
#include <vector>
#include <utility>
#include <map>


enum class AlarmType : uint8_t
{
  NONE,
  ALL_OK,
  NEED_SETTINGS,
  SENSOR_ERROR,
  ALL_THRESHOLDS_OUT,
  SOME_THRESHOLDS_OUT,
  CONNECTION_ERROR,
  INFLUX_ERROR,
  NO_SEND_DATA,
  NUM_ALARM_TYPES
};

struct LED {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct AlarmConfig
{
  bool testMode = false;
  bool ack = false;
  bool keepSpammingAfterAck = true; // TRUE = Mostra/Cicla comunque; FALSE = Spegni/Nascondi dopo ACK
};

struct AlarmState {
  bool isPresent = false; // Indica se l'allarme è attualmente presente
  bool isAcked = false;   // Indica se l'allarme è stato riconosciuto (acknowledged) dall'utente
};

class AlarmHandler{
  private:
    std::set<AlarmType> _activeAlarms;
    std::set<AlarmType>::iterator _currentIt;
    
    AlarmConfig _config;
    LED& _alarmLED;
    LCDHandler& _lcd;
    std::map<AlarmType, AlarmState> _alarmStates; // Mappa per tenere traccia dello stato di ogni allarme

  public:
    AlarmHandler(LCDHandler& lcd, LED& alarmLed);
    void begin(AlarmConfig config = AlarmConfig());
    void manageRoutineErrors(AlarmType alarm);
    void nextAlarm();
    void addAlarm(AlarmType type);
    void removeAlarm(AlarmType type);
    void clearAlarms();
    void setErrorsAcked();
    std::vector<AlarmType> getActiveAlarms();
    const std::map<AlarmType, AlarmState>& getAlarmStatus();
    AlarmConfig& getConfig ();
    LCDMsg getAlarmMessage(AlarmType type);

  private: 
    void ledOff();
    void setLedRGB(uint8_t r, uint8_t g, uint8_t b);
    
};

#endif