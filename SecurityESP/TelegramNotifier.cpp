#include "TelegramNotifier.h"
#include <stdio.h> // Richiesto per snprintf

TelegramNotifier::TelegramNotifier(const char* token, const char* chatId, const char* certRoot) 
    : _botToken(token), _chatId(chatId), _telegramCert(certRoot), _bot(token, _secureClient) {
}

void TelegramNotifier::begin() {
    configTime(0, 0, "it.pool.ntp.org");
    _secureClient.setTrustAnchors(&_telegramCert);
}

bool TelegramNotifier::sendNotification(const char* message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Impossibile inviare notifica Telegram: WiFi disconnesso.");
        return false;
    }
    
    Serial.print("Invio notifica Telegram... ");
    if (_bot.sendMessage(_chatId, message, "")) {
        Serial.println("Inviata con successo!");
        return true;
    } else {
        Serial.println("Errore nell'invio.");
        return false;
    }
}

bool TelegramNotifier::sendFlameAlert(bool isOnFlame, float temperature) {
    char buffer[150];

    const char* statusStr = isOnFlame ? "🚨 ATTENZIONE: Rilevato Incendio!" : "Stato Incendio: Rientrato nella norma.";
    snprintf(buffer, sizeof(buffer), 
             "🔥 *ALLARME INCENDIO*\n%s\nTemperatura attuale: %.1f °C", 
             statusStr, 
             temperature);

    return sendNotification(buffer);
}
