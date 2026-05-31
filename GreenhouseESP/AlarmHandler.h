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

class AlarmHandler{
  private:
    bool _enabled;
    std::set<AlarmType> _activeAlarms;
    std::set<AlarmType>::iterator _currentIt;

  public:
    AlarmHandler();
    void begin();
    void manageLEDerrors(AlarmType alarm);
    void nextAlarmColor();
    void flipEnabled();
    bool getAlarmStatus();
    void addAlarm(AlarmType type);
    void removeAlarm(AlarmType type);
    void clearAlarms();
    std::vector<AlarmType> getActiveAlarms();

  private: 
    void ledOff();
    void setLedRGB(uint8_t r, uint8_t g, uint8_t b);
};

#endif