#ifndef MOCK_CLASSES_H
#define MOCK_CLASSES_H

#include <string>

#include "../Interfaces/IAlarmHandler.h"
#include "../Interfaces/IInfluxHandler.h"
#include "../Interfaces/ILCDHandler.h"
#include "../Interfaces/IMqttHandler.h"
#include "../Interfaces/ISensorManager.h"



class MockAlarmHandler : public IAlarmHandler {
public:
    MockAlarmHandler() {}
    void setLedRGB(uint8_t r, uint8_t g, uint8_t b) override {
        std::string outp = "[MOCK LED] -->";
        
        if (r == 0 && g == 0 && b == 0) std::cout << outp << "OFF" << std::endl;
        else if (r == 1 && g == 0 && b == 0) std::cout << outp << "RED" << std::endl;
        else if (r == 0 && g == 1 && b == 0) std::cout << outp << "GREEN" << std::endl;
        else if (r == 0 && g == 0 && b == 1) std::cout << outp << "BLUE" << std::endl;
        else if (r == 1 && g == 1 && b == 0) std::cout << outp << "YELLOW" << std::endl;
        else if (r == 1 && g == 0 && b == 1) std::cout << outp << "PURPLE" << std::endl;
        else if (r == 0 && g == 1 && b == 1) std::cout << outp << "CYAN" << std::endl;
        else if (r == 1 && g == 1 && b == 1) std::cout << outp << "WHITE" << std::endl;
        }
    
        void ledOff() override {
            setLedRGB(0, 0, 0);
        }
};

class MockInfluxHandler : public IInfluxHandler {
    public:
        MockInfluxHandler() {}
        const char* getLastErrorMessage() override {return "MockInfluxHandler stirng";}
};

class MockLCDHandler : public ILCDHandler {
    public:
        MockLCDHandler() {}    
        void addMessage(const char *msgOne, const char *msgSec = "") override {}
};

class MockMqttHandler : public IMqttHandler {
    public:
        MockMqttHandler() {}
};

class MockSensorManager : public ISensorManager {
    public:
        MockSensorManager() {}
};
#endif // MOCK_CLASSES_H