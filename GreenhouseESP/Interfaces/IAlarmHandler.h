#ifndef I_ALARM_HANDLER_H
#define I_ALARM_HANDLER_H

#include <vector>
#include <set>
#include <cstdint>

enum class AlarmType {
    NONE,
    ALL_OK,
    SENSOR_ERROR,
    ALL_THRESHOLDS_OUT,
    SOME_THRESHOLDS_OUT,
    CONNECTION_ERROR,
    INFLUX_ERROR,
    NO_SEND_DATA,
    NUM_ALARM_TYPES
};

class IAlarmHandler {
    protected:
        bool _enabled;
        std::set<AlarmType> _activeAlarms;
        std::set<AlarmType>::iterator _currentIt;

    public:
        IAlarmHandler() : _enabled(true) {}
        virtual ~IAlarmHandler() {}

        void manageLEDerrors(AlarmType alarm) {
            switch (alarm) {
                case AlarmType::ALL_OK: setLedRGB(0, 1, 0); break; // Verde (LOW/HIGH o 0/1)
                case AlarmType::SOME_THRESHOLDS_OUT: setLedRGB(1, 0, 1); break; // Purple
                case AlarmType::ALL_THRESHOLDS_OUT: setLedRGB(1, 1, 0); break; // Giallo
                case AlarmType::SENSOR_ERROR: setLedRGB(1, 0, 0); break; // Rosso
                case AlarmType::CONNECTION_ERROR: setLedRGB(0, 0, 1); break; // Blu
                case AlarmType::INFLUX_ERROR: setLedRGB(1, 1, 1); break; // Bianco
                case AlarmType::NO_SEND_DATA: setLedRGB(0, 1, 1); break; // Azzurro
                default:
                    ledOff();
                    break;
            }
        }  
        void nextAlarmColor() {
            if (_activeAlarms.empty()) {
                if (_enabled) {
                    manageLEDerrors(AlarmType::ALL_OK);
                }
                else {
                    ledOff();
                }
                return;
            }

            if (!_enabled) {
                ledOff();
                return;
            }

            if (_currentIt == _activeAlarms.end()) {
                _currentIt = _activeAlarms.begin();
            }

            AlarmType current = *_currentIt;
            manageLEDerrors(current);
            _currentIt++;
        }

        void flipEnabled() {
            clearAlarms();
            _enabled = !_enabled;
        }
        
        bool getAlarmStatus() { return _enabled; }

        void addAlarm(AlarmType type) {
            if (type == AlarmType::NONE)
                return;
            _activeAlarms.insert(type);         // Se esiste già, non fa nulla.
            _currentIt = _activeAlarms.begin(); // Reset iteratore per sicurezza
        }
        void removeAlarm(AlarmType type){
            _activeAlarms.erase(type);          // Rimuove l'errore se presente
            _currentIt = _activeAlarms.begin(); // Reset iteratore
        }
        void clearAlarms() {
            _activeAlarms.clear();
            _currentIt = _activeAlarms.begin();
            ledOff();
        }

        std::vector<AlarmType> getActiveAlarms() {
            return std::vector<AlarmType>(_activeAlarms.begin(), _activeAlarms.end());
        }

    protected:
        virtual void setLedRGB(uint8_t r, uint8_t g, uint8_t b) = 0;
        virtual void ledOff() = 0;
};

#endif // I_ALARM_HANDLER_H