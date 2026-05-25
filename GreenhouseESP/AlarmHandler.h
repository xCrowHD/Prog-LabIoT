#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H

#include "Interfaces/IAlarmHandler.h" 

#include <set>
#include <vector>

class AlarmHandler : public IAlarmHandler {
private:
  bool _enabled;
  std::set<AlarmType> _activeAlarms;
  std::set<AlarmType>::iterator _currentIt;

public:
  AlarmHandler();
  void begin();  
private:
  void ledOff() override;
  void setLedRGB(uint8_t r, uint8_t g, uint8_t b) override;
};

#endif