#include "TelegramNotifier.h"
#include <stdio.h>  // Richiesto per snprintf

TelegramNotifier::TelegramNotifier(const char* token, const char* chatId)
  : _botToken(token), _chatId(chatId), _bot(token, _secureClient) {
}

void TelegramNotifier::begin() {
  configTime(0, 0, "it.pool.ntp.org", "time.nist.gov");
  _secureClient.setInsecure();
}

bool TelegramNotifier::sendNotification(const char* message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[Telegram] Impossibile inviare: WiFi disconnesso."));
    return false;
  }

  Serial.print(F("[Telegram] Connessione ai server di Telegram in corso... "));

  // Eseguiamo il tentativo di invio
  bool success = _bot.sendMessage(_chatId, message, "Markdown");

  if (success) {
    Serial.println(F("INVIATA CON SUCCESSO!"));
    Serial.println(F("--- [Telegram] FINE INVIO METEO METEO ---\n"));
    return true;
  } else {
    Serial.println(F("ERRORE NELL'INVIO!"));

    // 🔍 ANALISI DELL'ERRORE (I LOG CHE TI SERVONO)
    Serial.println(F("\n[Telegram Debug] --- ANALISI FALLIMENTO ---"));
    if (_secureClient.connected()) {
      Serial.println(F("[Telegram Debug] Il client è ancora connesso, ma il server ha rifiutato il payload."));
    } else {
      Serial.println(F("[Telegram Debug] Il client sicuro si è disconnesso bruscamente durante l'handshake."));
    }
    return false;
  }
}

bool TelegramNotifier::sendWeatherAlert(int weatherCode, const char* cityName) {
  // Aumentato a 320 per contenere comodamente il nome della città senza rischi di overflow
  char buffer[320];
  const char* pericoloStr = nullptr;
  bool isDangerous = true;

  switch (weatherCode) {
    case 51: pericoloStr = "🌧️ *Pioggerellina Leggera*"; break;
    case 53: pericoloStr = "🌧️ *Pioggerellina Moderata*"; break;
    case 55: pericoloStr = "🌧️ *Pioggerellina Densa*"; break;
    case 56: pericoloStr = "🥶❄️ *Pioviggine Gelata Leggera*"; break;
    case 57: pericoloStr = "🥶❄️ *Pioviggine Gelata Densa! Rischio gelate.*"; break;
    case 61: pericoloStr = "🌧️ *Pioggia Leggera*"; break;
    case 63: pericoloStr = "🌧️ *Pioggia Moderata*"; break;
    case 65: pericoloStr = "🌧️❗ *Pioggia Forte! Controllare infiltrazioni.*"; break;
    case 66: pericoloStr = "🧊❄️ *Pioggia Gelata Leggera*"; break;
    case 67: pericoloStr = "🧊❄️ *Pioggia Gelata Torrenziale! Pericolo ghiaccio strutturale.*"; break;
    case 71: pericoloStr = "❄️ *Nevicata Leggera*"; break;
    case 73: pericoloStr = "❄️ *Nevicata Moderata*"; break;
    case 75: pericoloStr = "❄️⚠️ *Forte Nevicata! Attenzione al peso sul tetto della serra.*"; break;
    case 77: pericoloStr = "❄️ *Precipitazione di Nevischio*"; break;
    case 80: pericoloStr = "🌦️ *Acquazzone Leggero*"; break;
    case 81: pericoloStr = "🌦️ *Acquazzone Moderato*"; break;
    case 82: pericoloStr = "⛈️⚠️ *Rovescio Violento! Rischio allagamento zona serra.*"; break;
    case 85: pericoloStr = "❄️ *Rovescio di Neve Leggero*"; break;
    case 86: pericoloStr = "❄️⚠️ *Rovescio di Neve Intenso*"; break;
    case 95: pericoloStr = "⚡ *Temporale in corso (Slight/Moderate)*"; break;
    case 96: pericoloStr = "⛈️❌ *Temporale con Grandine Leggera! Proteggere le colture.*"; break;
    case 99: pericoloStr = "🚨❌ *TEMPORALE CON FORTE GRANDINATA! Massimo pericolo per i teli/vetri della serra.*"; break;
    default:
      pericoloStr = "☀️ *Meteo Sicuro - Cielo Sereno*";
      isDangerous = false;
      break;
  }

  if (isDangerous) {
    snprintf(buffer, sizeof(buffer),
             "⚠️ *ALLERTA METEO SERRA*\n📍 Località: *%s*\n\nPrevisione per oggi:\n%s\n\n_Verificare lo stato di chiusura delle finestre._",
             cityName, pericoloStr);
  } else {
    snprintf(buffer, sizeof(buffer),
             "✅ *CHECK METEO SERRA*\n📍 Località: *%s*\n\nStato attuale:\n%s\n\n_Nessuna azione richiesta, la serra è al sicuro._",
             cityName, pericoloStr);
  }

  return sendNotification(buffer);
}