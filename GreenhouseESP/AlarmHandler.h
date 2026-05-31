#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H
#ifdef ARDUINO
  #include <Arduino.h>
  #define LED_RED D0
  #define LED_GREEN D4
  #define LED_BLUE D3
#else
  #include "MockLibraries/Serial.h"
  #include <cstdint>
  #include <iostream>
  #define LED_RED 11
  #define LED_BLUE 12
  #define LED_GREEN 13
#endif

#include <set>
#include <vector>


enum class AlarmType : uint8_t
{
  NONE,
  ALL_OK,
  SENSOR_ERROR,
  ALL_THRESHOLDS_OUT,
  SOME_THRESHOLDS_OUT,
  CONNECTION_ERROR,
  INFLUX_ERROR,
  NO_SEND_DATA,
  NUM_ALARM_TYPES
};

struct AlarmConfig {
  bool testMode = false;
  float disableTime = 20.0f; // Tempo a seguito del quale l'allarme torna a campionare i suoi errori
};

class AlarmHandler{
  private:
    bool _enabled;
    uint32_t _internalTime;       // Tempo corrente (reale su Arduino, simulato su PC)
    uint32_t _deactivationTimestamp; // Il momento in cui l'operatore ha disattivato l'allarme
    bool _inBlindPeriod;          // Flag: siamo nella finestra di blocco totale?

    std::set<AlarmType> _activeAlarms;
    std::set<AlarmType>::iterator _currentIt;
    
    AlarmConfig _config;

  public:
    AlarmHandler(AlarmConfig config);
    void begin();
    void manageLEDerrors(AlarmType alarm);
    void nextAlarmColor();
    void flipEnabled();
    bool getAlarmStatus();
    void addAlarm(AlarmType type);
    void removeAlarm(AlarmType type);
    void clearAlarms();
    std::vector<AlarmType> getActiveAlarms();
    AlarmConfig& getConfig ();

  private: 
    void ledOff();
    void setLedRGB(uint8_t r, uint8_t g, uint8_t b);
};

#endif