#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H

#include <Arduino.h>
#include <Ticker.h>

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
  void ledOff();

public:
  AlarmHandler();
  void begin();
  void setLedRGB(uint8_t r, uint8_t g, uint8_t b);
  void manageLEDerrors(AlarmType alarmType);
};

#endif