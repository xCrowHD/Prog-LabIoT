#include "AlarmHandler.h"

AlarmHandler::AlarmHandler() {
  _currentIt = _activeAlarms.begin();
}

void AlarmHandler::begin() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
}

void AlarmHandler::ledOff() {
  setLedRGB(LOW, LOW, LOW);
}

void AlarmHandler::manageLEDerrors(AlarmType alarm) {

  switch (alarm) {
    case AlarmType::ALL_OK:
      setLedRGB(LOW, HIGH, LOW);  // Verde
      break;
    case AlarmType::SOME_THRESHOLDS_OUT:
      setLedRGB(HIGH, LOW, HIGH);  // Purple
      break;
    case AlarmType::ALL_THRESHOLDS_OUT:
      setLedRGB(HIGH, HIGH, LOW);  // Giallo
      break;
    case AlarmType::SENSOR_ERROR:
      setLedRGB(HIGH, LOW, LOW);  // Rosso
      break;
    default:
      ledOff();
      return;  // Non aggiungiamo NONE allo storico dei "notificati"
  }
}

void AlarmHandler::setLedRGB(uint8_t r, uint8_t g, uint8_t b) {
  digitalWrite(LED_RED, r);
  digitalWrite(LED_GREEN, g);
  digitalWrite(LED_BLUE, b);
}

void AlarmHandler::addAlarm(AlarmType type) {
  if (type == AlarmType::NONE) return;
  _activeAlarms.insert(type);          // Se esiste già, non fa nulla. Comodo, no?
  _currentIt = _activeAlarms.begin();  // Reset iteratore per sicurezza
}

void AlarmHandler::removeAlarm(AlarmType type) {
  _activeAlarms.erase(type);           // Rimuove l'errore se presente
  _currentIt = _activeAlarms.begin();  // Reset iteratore
}

void AlarmHandler::nextAlarmColor() {
  if (_activeAlarms.empty()) {
    ledOff();
    return;
  }

  // Se l'iteratore è alla fine o non valido, ricomincia da capo
  if (_currentIt == _activeAlarms.end()) {
    _currentIt = _activeAlarms.begin();
  }

  AlarmType current = *_currentIt;
  manageLEDerrors(current);
  _currentIt++;
}

void AlarmHandler::clearAlarms() {
  _activeAlarms.clear();
  _currentIt = _activeAlarms.begin();
  ledOff();
}