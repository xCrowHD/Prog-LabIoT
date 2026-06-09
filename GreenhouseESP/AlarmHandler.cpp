#include "AlarmHandler.h"

AlarmHandler::AlarmHandler(LCDHandler &lcd, LED &alarmLed) : _lcd(lcd), _alarmLED(alarmLed)
{
  _currentIt = _activeAlarms.begin();
}

void AlarmHandler::manageRoutineErrors(AlarmType alarm)
{
  switch (alarm)
  {
  case AlarmType::ALL_OK:
    setLedRGB(LOW, HIGH, LOW);
    break; // Verde
  case AlarmType::NEED_SETTINGS:
    setLedRGB(HIGH, HIGH, HIGH);
    break;
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
    setLedRGB(LOW, LOW, HIGH);
    break; // Blu
  case AlarmType::NO_SEND_DATA:
    setLedRGB(LOW, HIGH, HIGH);
    break; // Azzurro
  case AlarmType::NONE:
    ledOff();
    break;
  }
}

void AlarmHandler::nextAlarm()
{
  // 1. GESTIONE TRIGGER ACK GLOBALE
  if (_config.ack == true)
  {
    setAllAlarmAcked();
    _config.ack = false;
  }

  // 2. ROUTINE LED
  // Se la lista attiva è vuota, significa che non ci sono errori oppure che sono stati tutti ackati.
  // In entrambi i casi, per il LED è una situazione nominale (Verde o Spento, in base a come preferite ALL_OK)
  if (_activeAlarms.empty())
  {
    manageRoutineErrors(AlarmType::ALL_OK);
    return;
  }

  // Avanzamento ciclo LED originale (solo sugli allarmi rimasti non-ackati)
  if (_currentIt == _activeAlarms.end())
  {
    _currentIt = _activeAlarms.begin();
  }

  AlarmType current = *_currentIt;
  manageRoutineErrors(current);
  _currentIt++;
}

void AlarmHandler::addAlarm(AlarmType type)
{
  if (type == AlarmType::NONE)
  {
    _activeAlarms.erase(AlarmType::NONE);
    return;
  }

  AlarmState &state = _alarmStates[type];
  state.isPresent = true;

  // Gestione LCD: notifica se non è ackato OPPURE se vogliamo continuare a spammare a schermo
  if ((!state.isAcked) || _config.keepSpammingAfterAck)
  {
      auto msg = getAlarmMessage(type);
      if (strlen(msg.firstLine) > 0)
      {
        _lcd.addMessage(msg.firstLine, msg.secondLine, msg.type);
      }
      //state.lcdNotified = true;
  }

  // Gestione LED: Il LED si attiva SOLO se l'allarme NON è ancora stato ackato
  if (!state.isAcked)
  {
    if (_activeAlarms.find(type) == _activeAlarms.end())
    {
      _activeAlarms.insert(type);
    }
  }
}

void AlarmHandler::removeAlarm(AlarmType type)
{
  // 1. Aggiorna lo stato logico
  if (_alarmStates.find(type) != _alarmStates.end())
  {
    _alarmStates[type].isPresent = false;
    _alarmStates[type].isAcked = false;
  }

  // 2. Rimuovi i messaggi dall'LCD
  auto msg = getAlarmMessage(type);
  _lcd.removeMessage(msg.firstLine, msg.secondLine);

  // 3. CONTROLLO CHIRURGICO DEL "BUCO":
  // Verifichiamo se l'allarme che stiamo per cancellare è PROPRIO quello 
  // che l'iteratore sta puntando in questo momento.
  bool deletingCurrentAlarm = (_currentIt != _activeAlarms.end() && *_currentIt == type);

  // 4. Cancellazione fisica dall'insieme
  _activeAlarms.erase(type);

  // 5. Risoluzione del puntatore orfano
  if (deletingCurrentAlarm || _currentIt == _activeAlarms.end())
  {
    // Se abbiamo fatto un buco sotto i piedi dell'iteratore, lo riportiamo al sicuro all'inizio
    _currentIt = _activeAlarms.begin();
  }
  // Se invece abbiamo cancellato un allarme che era "indietro" o "avanti" rispetto a dove 
  // si trova il LED adesso, non tocchiamo _currentIt. Rimane dove si trova e continua a ciclatre!
}

void AlarmHandler::setAllAlarmAcked()
{
  for (auto &[alarmType, state] : _alarmStates)
  {
    if (state.isPresent && !state.isAcked)
    {
      state.isAcked = true;

      // Se NON dobbiamo spammare sull'LCD, rimuoviamo il messaggio adesso
      if (!_config.keepSpammingAfterAck)
      {
        auto msg = getAlarmMessage(alarmType);
        _lcd.removeMessage(msg.firstLine, msg.secondLine);
      }

      // IL LED SI SPEGNE SEMPRE: Cancelliamo tassativamente l'allarme dal ciclo LED
      _activeAlarms.erase(alarmType);
    }
  }
  _currentIt = _activeAlarms.begin();
}

void AlarmHandler::clearAlarms()
{
  _activeAlarms.clear();
  _alarmStates.clear();
  _lcd.clearErrors();
  _currentIt = _activeAlarms.begin();
  ledOff();
}

std::vector<AlarmType> AlarmHandler::getActiveAlarms()
{
  return std::vector<AlarmType>(_activeAlarms.begin(), _activeAlarms.end());
}

void AlarmHandler::begin(AlarmConfig config)
{
  _config = config;
  pinMode(_alarmLED.r, OUTPUT);
  pinMode(_alarmLED.g, OUTPUT);
  pinMode(_alarmLED.b, OUTPUT);
}

void AlarmHandler::ledOff()
{
  setLedRGB(LOW, LOW, LOW);
}

void AlarmHandler::setLedRGB(uint8_t r, uint8_t g, uint8_t b)
{
  digitalWrite(_alarmLED.r, r);
  digitalWrite(_alarmLED.g, g);
  digitalWrite(_alarmLED.b, b);

#ifndef ARDUINO
  const char *mk = "[MOCK PIN] --> ";
  if (_config.testMode)
  {
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

AlarmConfig &AlarmHandler::getConfig() { return _config; }

LCDMsg AlarmHandler::getAlarmMessage(AlarmType type)
{
  switch (type)
  {
  case AlarmType::ALL_OK:
    return LCDMsg { "Status", "All OK", MessageType::INFO };
  case AlarmType::NEED_SETTINGS:
    return LCDMsg {"Status", "Need settings", MessageType::INFO};
  case AlarmType::SOME_THRESHOLDS_OUT:
    return LCDMsg{"WARNING: Some", "thresholds O.O.R", MessageType::WARNING};
  case AlarmType::ALL_THRESHOLDS_OUT:
    return LCDMsg{"WARNING: All", "thresholds O.O.R", MessageType::WARNING};
  case AlarmType::SENSOR_ERROR:
    return LCDMsg{"ERROR", "Sensor error", MessageType::ERROR};
  case AlarmType::CONNECTION_ERROR:
    return LCDMsg{"ERROR", "Connection error", MessageType::ERROR};
  case AlarmType::INFLUX_ERROR:
    return LCDMsg{"ERROR", "Influx error", MessageType::ERROR};
  case AlarmType::NO_SEND_DATA:
    return LCDMsg{"ERROR", "Missing data", MessageType::ERROR};
  default:
    return LCDMsg{"", "", MessageType::INFO};
  }
}