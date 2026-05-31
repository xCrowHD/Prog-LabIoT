#include "AlarmHandler.h"

AlarmHandler::AlarmHandler() {
  _currentIt = _activeAlarms.begin();
  _enabled = true;
}

void AlarmHandler::manageLEDerrors(AlarmType alarm)
{
  switch (alarm)
  {
  case AlarmType::ALL_OK:
    setLedRGB(LOW, HIGH, LOW);
    break; // Verde (LOW/HIGH o 0/1)
  case AlarmType::SOME_THRESHOLDS_OUT:
    setLedRGB(HIGH, LOW, HIGH);
    break; // Purple
  case AlarmType::ALL_THRESHOLDS_OUT:
    setLedRGB(HIGH, HIGH, LOW);
    break; // Giallo
  case AlarmType::SENSOR_ERROR:
    setLedRGB(HIGH, LOW, LOW);
    break; // Rosso
  case AlarmType::CONNECTION_ERROR:
    setLedRGB(LOW, LOW, HIGH);
    break; // Blu
  case AlarmType::INFLUX_ERROR:
    setLedRGB(HIGH, HIGH, HIGH);
    break; // Bianco
  case AlarmType::NO_SEND_DATA:
    setLedRGB(LOW, HIGH, HIGH);
    break; // Azzurro
  default:
    ledOff();
    break;
  }
}

void AlarmHandler::nextAlarmColor()
{
  if (_activeAlarms.empty())
  {
    if (_enabled)
    {
      manageLEDerrors(AlarmType::ALL_OK);
    }
    else
    {
      ledOff();
    }
    return;
  }

  if (!_enabled)
  {
    ledOff();
    return;
  }

  if (_currentIt == _activeAlarms.end())
  {
    _currentIt = _activeAlarms.begin();
  }

  AlarmType current = *_currentIt;
  manageLEDerrors(current);
  _currentIt++;
}

void AlarmHandler::flipEnabled()
{
  clearAlarms();
  _enabled = !_enabled;
}

bool AlarmHandler::getAlarmStatus() { return _enabled; }

void AlarmHandler::addAlarm(AlarmType type)
{
  
  if (type == AlarmType::NONE)
    return;
  //std::cout << "Alarm added: " << static_cast<int>(type) << std::endl;
  _activeAlarms.insert(type);         // Se esiste già, non fa nulla.
  _currentIt = _activeAlarms.begin(); // Reset iteratore per sicurezza
}

void AlarmHandler::removeAlarm(AlarmType type)
{
  //std::cout << "Alarm removed: " << static_cast<int>(type) << std::endl;
  _activeAlarms.erase(type);          // Rimuove l'errore se presente
  _currentIt = _activeAlarms.begin(); // Reset iteratore
}
void AlarmHandler::clearAlarms()
{
  _activeAlarms.clear();
  _currentIt = _activeAlarms.begin();
  ledOff();
}

std::vector<AlarmType> AlarmHandler::getActiveAlarms()
{
  return std::vector<AlarmType>(_activeAlarms.begin(), _activeAlarms.end());
}

void AlarmHandler::begin() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
}

void AlarmHandler::ledOff() {
  setLedRGB(LOW, LOW, LOW);
}

void AlarmHandler::setLedRGB(uint8_t r, uint8_t g, uint8_t b) {
  digitalWrite(LED_RED, r);
  digitalWrite(LED_GREEN, g);
  digitalWrite(LED_BLUE, b);
}