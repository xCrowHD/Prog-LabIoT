#ifndef I_SENSOR_MANAGER_H
#define I_SENSOR_MANAGER_H

struct PlantData
{
    float temperature;
    float humidity;
    int light;
    bool valid;
};

class ISensorManager {
    public:
        virtual ~ISensorManager() {}
};

#endif // I_SENSOR_MANAGER_H