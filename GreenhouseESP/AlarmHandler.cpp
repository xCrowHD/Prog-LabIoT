#include "AlarmHandler.h"

AlarmHandler::AlarmHandler(AlarmConfig config) : _config(config), _enabled(true) {
  _currentIt = _activeAlarms.begin();
  _internalTime = 0;
  _deactivationTimestamp = 0;
  _inBlindPeriod = false;
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
  if (!_enabled)
  {
    ledOff();
    return;
  }

  if (_activeAlarms.empty())
  {
    manageLEDerrors(AlarmType::ALL_OK);
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
    _internalTime = millis();

  if (_enabled){
    _enabled = false;
    _deactivationTimestamp = _internalTime; // Registra il momento dello spegnimento
    _inBlindPeriod = true;                  // Attiva la finestra di cecità totale
    clearAlarms();
  }
  else{
    _enabled = true;
    _inBlindPeriod = false;
  }
}

bool AlarmHandler::getAlarmStatus() { return _enabled; }

void AlarmHandler::addAlarm(AlarmType type)
{
  if (type == AlarmType::NONE) return;

  _internalTime = millis();

  // FASE 1: Se siamo nel periodo di cecità totale, controlliamo se è scaduto
  if (_inBlindPeriod) {
    if (_internalTime - _deactivationTimestamp < _config.disableTime) {
      // Il delayTime NON è ancora passato: ignora totalmente l'allarme (non salvare nulla)
      return; 
    } else {
      // Il delayTime è SCADUTO! Finiamo il periodo di cecità
      _inBlindPeriod = false; 
    }
  }

  // FASE 2: Se il sistema è disabilitato (ma la cecità è scaduta), 
  // il codice prosegue qui sotto. L'allarme viene salvato nel set, 
  // ma NON verrà mostrato perché i LED saranno gestiti di conseguenza.
  
  _activeAlarms.insert(type);
  _currentIt = _activeAlarms.begin();
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

  #ifndef ARDUINO // Evito che il codice venga compilato e inserito sull'ESP per risparmiare memoria!
    const char* mk = "[MOCK PIN] --> ";
    if (_config.testMode) {
        if (r == 0 && g == 0 && b == 0)
          std::cout << mk << "OFF" << std::endl;
        else if (r == 0 && g == 0 && b == 1)
          std::cout << mk << "BLUE" << std::endl;
        else if (r == 0 && g == 1 && b == 0)
          std::cout << mk << "GREEN" << std::endl;
        else if (r == 1 && g == 0 && b == 0)
          std::cout << mk << "RED" << std::endl;
        else if (r == 0 && g == 1 && b == 1)
          std::cout << mk << "CYAN" << std::endl;
        else if (r == 1 && g == 0 && b == 1)
          std::cout << mk << "PURPLE" << std::endl;
        else if (r == 1 && g == 1 && b == 0)
          std::cout << mk << "YELLOW" << std::endl;
        else if (r == 1 && g == 1 && b == 1)
          std::cout << mk << "WHITE" << std::endl;
    }
  #endif
}
AlarmConfig& AlarmHandler::getConfig() { return _config; }