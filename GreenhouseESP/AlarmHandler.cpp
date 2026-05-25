#include "AlarmHandler.h"
#include <Arduino.h>

#define LED_RED D0
#define LED_GREEN D4
#define LED_BLUE D3

AlarmHandler::AlarmHandler() {
  _currentIt = _activeAlarms.begin();
  _enabled = true;
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