#include "AlarmHandler.h"

AlarmHandler::AlarmHandler() {}

void AlarmHandler::begin() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
}

void AlarmHandler::ledOff() {
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
}

void AlarmHandler::manageLEDerrors(AlarmType alarm) {

  ledOff();
  // Se arriviamo qui, l'allarme è nuovo
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