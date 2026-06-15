#ifndef TELEGRAM_NOTIFIER_H
#define TELEGRAM_NOTIFIER_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

class TelegramNotifier {
private:
  const char* _botToken;
  const char* _chatId;
  WiFiClientSecure _secureClient;
  UniversalTelegramBot _bot;

public:
  TelegramNotifier(const char* token, const char* chatId);

  void begin();
  bool sendNotification(const char* message);
  bool sendWeatherAlert(int weatherCode, const char* cityName);
};

#endif