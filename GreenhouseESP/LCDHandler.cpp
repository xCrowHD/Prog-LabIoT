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

void LCDHandler::displayData(float temp, float hum, float lux) {

  _lcd.clear();
  _lcd.home();
  _lcd.print("T:");
  _lcd.print((int)temp);
  _lcd.print("C ");
  _lcd.print("H:");
  _lcd.print((int)hum);
  _lcd.print("%");  // Spazi extra per pulire residui

  // Riga 1: Lux
  _lcd.setCursor(0, 1);
  _lcd.print("Luce: ");
  _lcd.print((int)lux);
  _lcd.print(" lx");
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