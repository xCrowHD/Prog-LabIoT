#include "LCDHandler.h" 

LCDHandler::LCDHandler(LCDConfig config)
  : _lcd(DISPLAY_ADDR, DISPLAY_CHARS, DISPLAY_LINES), _config(config) {}

void LCDHandler::begin() {
  Wire.begin();
  Wire.beginTransmission(DISPLAY_ADDR);
  if (_config.testMode) {
    _queue.clear(); // da togliere se si vuole mantenere la coda dei messaggi anche dopo un reset, ma per ora preferisco svuotarla per evitare confusione nei test
  }
  
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

void LCDHandler::addMessage(const char* msgOne, const char* msgSec, MessageType type) {
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
  newMsg.type = type;

  _queue.push_back(newMsg);
}

void LCDHandler::removeMessage(const char *msgOne, const char *msgSec)
{
  // Rimuove tutti i messaggi nella deque che corrispondono a msgOne e msgSec
  _queue.erase(
      std::remove_if(_queue.begin(), _queue.end(),
                     [msgOne, msgSec](const LCDMsg &m)
                     {
                       return (strcmp(m.firstLine, msgOne) == 0 && strcmp(m.secondLine, msgSec) == 0);
                     }),
      _queue.end());

  // Se la deque si svuota, puoi decidere di resettare lo schermo o mostrare un messaggio di default
}

void LCDHandler::clearErrors() {
  _queue.erase(std::remove_if(_queue.begin(), _queue.end(),
    [](const LCDMsg& msg) { return msg.type == MessageType::ERROR; }),
    _queue.end());
}

void LCDHandler::popAndDisplay() {
  if (_queue.empty()){
    if (_config.testMode) {
      Serial.print("[Empty]");
    }
    return;
  } 

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
  addMessage(row1, row2, MessageType::DATA);
}

LCDConfig& LCDHandler::getConfig() {
  return _config;
}

std::deque<LCDMsg> LCDHandler::getQueue() {
  return _queue;
}