#ifndef I_MQTT_HANDLER
#define I_MQTT_HANDLER

struct Thresholds
{
    char plantName[32] = "";
    float tempMin = 0;
    float tempMax = 0;
    float humMin = 0;
    float humMax = 0;
    int luxMin = 0;
    int luxMax = 0;
};

struct Topics
{
    char threshold[32] = "";
    char running[32] = "";
    char set[32] = "";
    char backup[32] = "";
};

struct McuSettings
{
    char mcuName[32] = "";
    bool isBackup = false;
    float timer = 0;
};

enum class Status
{
    ONLINE,
    CONNECTING,
    OFFLINE,
};

class IMqttHandler {
    public:
        virtual ~IMqttHandler() {}
};

#endif // I_MQTT_HANDLER_H