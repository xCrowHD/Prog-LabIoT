#ifndef MOCK_DHT_H
#define MOCK_DHT_H

#include <cstdint>
#include <iostream>

class DHT {
    public:
        DHT(uint8_t dhtPin, uint8_t dhtType){
            std::cout << "DHT constructor called, remember to call begin()" << std::endl;
        }
        void begin(){
            std::cout << "DHT begin(" << std::endl;
        }
        float readHumidity(){return 0.0f;}
        float readTemperature(){return 0.0f;}
};

#endif // MOCK_DHT_H