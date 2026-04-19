#ifndef LCD_HANDLER_H
#define LCD_HANDLER_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define DISPLAY_CHARS 16
#define DISPLAY_LINES 2
#define DISPLAY_ADDR 0x27

class LCDHandler {
private:
  LiquidCrystal_I2C _lcd;
public:
  LCDHandler();
  void begin();
  void displayData(float temp, float hum, float lux);
  void displayMessage(const char* line1, const char* line2 = "");
};

#endif