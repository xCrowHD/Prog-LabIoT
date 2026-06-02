#ifndef MOCK_WIFI_MANAGER_H
#define MOCK_WIFI_MANAGER_H

#include <iostream>

class WiFiManager
{
public:
    WiFiManager() {}
    bool autoConnect()
    {
        std::cout << "WiFiManager: autoConnect() chiamato. Connessione simulata riuscita!" << std::endl;
        return true;
    }

    bool autoConnect(const char *apName, const char *apPassword = nullptr)
    {
        std::cout << "WiFiManager: autoConnect() chiamato per l'AP: " << apName << std::endl;
        return true;
    }
    void resetSettings()
    {
        std::cout << "WiFiManager: resetSettings() chiamato. Credenziali WiFi cancellate!" << std::endl;
    }
};

#endif // MOCK_WIFI_MANAGER_H