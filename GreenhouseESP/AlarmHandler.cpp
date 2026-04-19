#include "AlarmHandler.h"

AlarmHandler::AlarmHandler(int r, int g, int b) : pinR(r), pinG(g), pinB(b) {
    _alarmType = NONE;
    _blinkRemaining = 0;
    _valR = _valG = _valB = 0;
    _silenced = false;
    _iStatus = 0;

    clearPStatus(); // Inizializza l'array a NONE
    ledOff();
}

void AlarmHandler::ledOff() {
    digitalWrite(pinR, LOW);
    digitalWrite(pinG, LOW);
    digitalWrite(pinB, LOW);
}

void AlarmHandler::startBlink(int r, int g, int b, int count) {
    _tickerBlink.detach(); 

    // Convertiamo 0-255 in 0-1023 (range ESP8266)
    this->_valR = map(r, 0, 255, 0, 1023);
    this->_valG = map(g, 0, 255, 0, 1023);
    this->_valB = map(b, 0, 255, 0, 1023);

    this->_blinkRemaining = count * 2;
    _tickerBlink.attach(0.2, &AlarmHandler::handleBlinkWrapper, this);
}

void AlarmHandler::applyBlinkState(bool on) {
    if (on) {
        analogWrite(pinR, _valR);
        analogWrite(pinG, _valG);
        analogWrite(pinB, _valB);
    } else {
        ledOff();
    }
}

void AlarmHandler::handleBlinkWrapper(AlarmHandler* instance) {
    if (instance->_blinkRemaining > 0) {
        bool state = (instance->_blinkRemaining % 2 != 0);
        instance->applyBlinkState(state); // Questa gestisce tutto l'RGB
        instance->_blinkRemaining--;
    } else {
        instance->_tickerBlink.detach();
        instance->ledOff();
    }
}

void AlarmHandler::clearPStatus() {
    for (int i = 0; i < NUM_ALARM_TYPES; i++) {
        _pStatus[i] = NONE;
    }
    _iStatus = 0;
}

void AlarmHandler::manageLEDerrors() {
    // Se silenziato o se questo specifico allarme è già stato notificato, usciamo
    if (_silenced || isAlreadyNotified(_alarmType)) {
        return; 
    }

    // Se arriviamo qui, l'allarme è nuovo
    switch (_alarmType) {
        case ALL_OK:
            startBlink(0, 255, 0, 2); 
            break;
        case SOME_THRESHOLDS_OUT:
            startBlink(255, 255, 0, 3); // Giallo
            break;
        case ALL_THRESHOLDS_OUT:
            startBlink(255, 128, 0, 3); // Arancione
            break;
        case SENSOR_ERROR:
            analogWrite(pinR, 1023); // Rosso fisso
            break;
        default:
            ledOff();
            return; // Non aggiungiamo NONE allo storico dei "notificati"
    }

    // AGGIUNGIAMO ALLO STORICO SOLO ORA
    if (_iStatus < NUM_ALARM_TYPES) {
        _pStatus[_iStatus] = _alarmType;
        _iStatus++;
    }
}

bool AlarmHandler::isAlreadyNotified(AlarmType type) {
    for (int i = 0; i < NUM_ALARM_TYPES; i++) {
        if (_pStatus[i] == type) return true;
    }
    return false;
}

void AlarmHandler::setAlarmType(AlarmType type) {
    _alarmType = type;
}

void AlarmHandler::setStatus(bool silence) 
{
  _silenced = silence;
}

bool AlarmHandler::getStatus()
{
  return _silenced;
}