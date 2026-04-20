#ifndef LCD_HANDLER_H
#define LCD_HANDLER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <deque>

#define DISPLAY_CHARS 16
#define DISPLAY_LINES 2
#define DISPLAY_ADDR 0x27

struct LCDMsg{
  char firstLine[DISPLAY_CHARS + 1];
  char secondLine[DISPLAY_CHARS + 1];
};

class LCDHandler {
private:
  LiquidCrystal_I2C _lcd;
  std::deque<LCDMsg> _queue;
public:
  LCDHandler();
  void begin();
  void popAndDisplay();
  void addMessage(const char* msgOne, const char* msgSec = "");
  void addMessagePlantData(float temp, float hum, float lux);
private:
  void displayMessage(const char* line1, const char* line2 = "");
};

#endif