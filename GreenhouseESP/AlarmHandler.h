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
  #define LED_RED 0
  #define LED_BLUE 4
  #define LED_GREEN 3
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
  bool enable = true;
};

class AlarmHandler{
  private:

    std::set<AlarmType> _activeAlarms;
    std::set<AlarmType>::iterator _currentIt;
    
    AlarmConfig _config;

  public:
    AlarmHandler(AlarmConfig config = AlarmConfig());
    void begin();
    void manageLEDerrors(AlarmType alarm);
    void nextAlarmColor();
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