#include "TelegramNotifier.h"
#include <stdio.h> // Richiesto per snprintf

TelegramNotifier::TelegramNotifier(const char* token, const char* chatId) 
    : _botToken(token), _chatId(chatId), _telegramCert(TELEGRAM_CERTIFICATE_ROOT), _bot(token, _secureClient) {
}

void TelegramNotifier::begin() {
    configTime(0, 0, "it.pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    while (now < 24 * 3600) { 
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println(" Sincronizzato!");
    _secureClient.setTrustAnchors(&_telegramCert);
}

bool TelegramNotifier::sendNotification(const char* message) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Impossibile inviare notifica Telegram: WiFi disconnesso.");
        return false;
    }
    
    Serial.print("Invio notifica Telegram... ");
    if (_bot.sendMessage(_chatId, message, "Markdown")) {
        Serial.println("Inviata con successo!");
        return true;
    } else {
        Serial.println("Errore nell'invio.");
        return false;
    }
}

bool TelegramNotifier::sendFlameAlert(bool isOnFlame, float temperature) {
    char buffer[256];

    // Cambiamo dinamicamente sia il titolo che la descrizione
    const char* titleStr = isOnFlame ? "🔥 *ALLARME INCENDIO ATTIVO*" : "✅ *RIPRISTINO EMERGENZA INCENDIO*";
    const char* statusStr = isOnFlame ? "🚨 Rilevata fiamma e temperatura critica!" : "🟢 La situazione è rientrata nella norma.";

    snprintf(buffer, sizeof(buffer), 
             "%s\n%s\n🔸 Temperatura attuale: %.1f °C", 
             titleStr, 
             statusStr, 
             temperature);

    return sendNotification(buffer);
}
