#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H

#include <Arduino.h>
#include <set>

#define LED_RED D0
#define LED_GREEN D4
#define LED_BLUE D3

enum class AlarmType { NONE,
                 ALL_OK,
                 SENSOR_ERROR,
                 ALL_THRESHOLDS_OUT,
                 SOME_THRESHOLDS_OUT,
                 NO_SEND_DATA,
                 NUM_ALARM_TYPES };  //NUM_ALARM_TYPE = #elementi di enum

class AlarmHandler {
private:
  AlarmType _alarmType;
  std::set<AlarmType> _activeAlarms; // Il set gestisce i duplicati da solo
  std::set<AlarmType>::iterator _currentIt; // Iteratore per scorrere

public:
  AlarmHandler();
  void begin();
  void addAlarm(AlarmType type);
  void removeAlarm(AlarmType type);
  void nextAlarmColor();
  void clearAlarms();

private:
  void ledOff();
  void setLedRGB(uint8_t r, uint8_t g, uint8_t b);
  void manageLEDerrors(AlarmType alarmType);
};

#endif