#include "LCDHandler.h"

LCDHandler::LCDHandler()
  : _lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES) {}

void LCDHandler::begin() {
  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(F("LCD found."));
    _lcd.begin(DISPLAY_CHARS, 2);  // initialize the lcd
    _lcd.setBacklight(255);
    _lcd.clear();
    _lcd.home();


  } else {
    Serial.print(F("LCD not found. Error "));
    Serial.println(error);
    Serial.println(F("Check connections and configuration. Reset to try again!"));
    while (true)
      delay(1);
  }
}

void LCDHandler::displayMessage(const char* line1, const char* line2) {
  _lcd.clear();
  _lcd.home();
  _lcd.print(line1);
  if (line2[0] != '\0') {
    _lcd.setCursor(0, 1);
    _lcd.print(line2);
  }
}

void LCDHandler::addMessage(const char* msgOne, const char* msgSec) {
  for (const auto& item : _queue) {
    if (strcmp(item.firstLine, msgOne) == 0 && strcmp(item.secondLine, msgSec) == 0) {
      return;  // Già in coda, esco
    }
  }

  LCDMsg newMsg;
  strncpy(newMsg.firstLine, msgOne, DISPLAY_CHARS);
  strncpy(newMsg.secondLine, msgSec, DISPLAY_CHARS);
  newMsg.firstLine[DISPLAY_CHARS] = '\0';
  newMsg.secondLine[DISPLAY_CHARS] = '\0';

  _queue.push_back(newMsg);
}

void LCDHandler::popAndDisplay() {
  if (_queue.empty()) return;

  // Prendo il primo elemento
  LCDMsg msgToShow = _queue.front();
  _queue.pop_front();

  _lcd.clear();
  _lcd.setCursor(0, 0);
  displayMessage(msgToShow.firstLine, msgToShow.secondLine);
}

void LCDHandler::addMessagePlantData(float temp, float hum, float lux) {
  char row1[17];  // Buffer per riga 1
  char row2[17];  // Buffer per riga 2

  // Formattiamo i dati come char*
  snprintf(row1, sizeof(row1), "T:%dC H:%d%%", (int)temp, (int)hum);
  snprintf(row2, sizeof(row2), "Luce: %d lx", (int)lux);

  // Aggiungiamo i char* alla coda del display
  addMessage(row1, row2);
}