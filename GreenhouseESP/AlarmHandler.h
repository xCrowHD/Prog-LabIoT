#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H

#ifdef ARDUINO
#include <Arduino.h> // Vero ambiente Arduino sulla scheda
#else
#include "MockLibraries/Arduino.h" // Il finto Arduino.h che abbiamo appena creato sul PC
#endif
#include "MqttHandler.h"

#include <set>
#include <vector>

enum class AlarmType { NONE,
                 ALL_OK,
                 SENSOR_ERROR,
                 ALL_THRESHOLDS_OUT,
                 SOME_THRESHOLDS_OUT,
                 CONNECTION_ERROR,
                 INFLUX_ERROR,
                 NO_SEND_DATA,
                 NUM_ALARM_TYPES };  //NUM_ALARM_TYPE = #elementi di enum

class AlarmHandler {
private:
  bool _enabled;
  std::set<AlarmType> _activeAlarms; // Il set gestisce i duplicati da solo
  std::set<AlarmType>::iterator _currentIt; // Iteratore per scorrere

public:
  AlarmHandler();
  void begin();
  void addAlarm(AlarmType type);
  void removeAlarm(AlarmType type);
  void nextAlarmColor();
  void flipEnabled();
  bool getAlarmStatus();
  std::vector<AlarmType> getActiveAlarms();
  void clearAlarms();

private:
  void ledOff();
  void setLedRGB(uint8_t r, uint8_t g, uint8_t b);
  void manageLEDerrors(AlarmType alarmType);
};

#endif