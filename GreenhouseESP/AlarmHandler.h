#ifndef ALARMHANDLER_H
#define ALARMHANDLER_H

#include <Arduino.h>
#include <Ticker.h>

enum AlarmType {NONE, ALL_OK, SENSOR_ERROR, ALL_THRESHOLDS_OUT, SOME_THRESHOLDS_OUT, NO_SEND_DATA, NUM_ALARM_TYPES}; //NUM_ALARM_TYPE = #elementi di enum

class AlarmHandler {
  private:
    int pinR, pinG, pinB; 
    int _valR, _valG, _valB; //range 0-1023 per combinare diversi tipi di colori

    AlarmType _pStatus[NUM_ALARM_TYPES];
    int _iStatus;

    AlarmType _alarmType;
    bool _silenced;

    int _blinkRemaining;
    int _currentBlinkPin;
    Ticker _tickerBlink;

    static void handleBlinkWrapper(AlarmHandler* instance);

    void applyBlinkState(bool on);
    void ledOff();

    void clearPStatus();

  public:
    AlarmHandler(int r, int g, int b);
    void setAlarmType(AlarmType type);
    bool isAlreadyNotified(AlarmType type);
    void manageLEDerrors(); // Assicurati che il nome sia identico ovunque
    void startBlink(int r, int g, int b, int count); //valori 0-255
    void setStatus(bool silence);
    bool getStatus();
  
};

#endif