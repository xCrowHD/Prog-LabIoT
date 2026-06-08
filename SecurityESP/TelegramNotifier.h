#ifndef TELEGRAM_NOTIFIER_H
#define TELEGRAM_NOTIFIER_H

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

class TelegramNotifier {
private:
    const char* _botToken;
    const char* _chatId;
    X509List _telegramCert;
    WiFiClientSecure _secureClient;
    UniversalTelegramBot _bot;

public:
    TelegramNotifier(const char* token, const char* chatId);

    void begin();
    bool sendNotification(const char* message);
    bool sendFlameAlert(bool isOnFlame, float temperature);
};

#endif