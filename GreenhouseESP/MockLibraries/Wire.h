#ifndef MOCK_WIRE_H
#define MOCK_WIRE_H

typedef unsigned char byte;

class MockWire {
    public: 
        void begin (){};
        void beginTransmission(int addr){}
        byte endTransmission() {return 1;}
};

inline MockWire Wire;
#endif // MOCK_WIRE_H