#ifndef LCD_HANDLER_H
#define LCD_HANDLER_H
#include <deque>
#include <algorithm>

#ifdef ARDUINO
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#else
#include "MockLibraries/Serial.h"
#include "MockLibraries/LiquidCrystal_I2C.h"
#include "MockLibraries/Wire.h"
#endif


#define DISPLAY_CHARS 16
#define DISPLAY_LINES 2
#define DISPLAY_ADDR 0x27

enum class MessageType
{
  DATA,
  INFO,
  ERROR,
  WARNING
};

struct LCDMsg{
  char firstLine[DISPLAY_CHARS + 1];
  char secondLine[DISPLAY_CHARS + 1];
  MessageType type;
};

struct LCDConfig
{
  bool testMode = false;
};

class LCDHandler{
private:
  LiquidCrystal_I2C _lcd;
  std::deque<LCDMsg> _queue;

  LCDConfig _config;

public:
  LCDHandler(LCDConfig config = LCDConfig());
  void begin(); 
  void popAndDisplay();
  void addMessage(const char* msgOne, const char* msgSec = "", MessageType type = MessageType::INFO);
  void removeMessage(const char* msgOne, const char* msgSec = "");
  void addMessagePlantData(float temp, float hum, float lux);
  void clearAll();
  void clearErrors();
  LCDConfig& getConfig();
  std::deque<LCDMsg> getQueue();
  
private: 
  void displayMessage(const char *line1, const char *line2 = "");
};

#endif // LCD_HANDLER_H