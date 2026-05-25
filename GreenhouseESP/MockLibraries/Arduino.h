#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <iostream>

#define F(x) x

class SerialMock {
    public:
        void begin(int baudrate) {}
        void print(const char *str)
        {
            std::cout << str; 
        }

        void println(const char *str)
        {
            std::cout << str << std::endl;
        }

        void print(int num) { std::cout << num; }
        void println(int num) { std::cout << num << std::endl; }
};

inline SerialMock Serial;
#endif // ARDUINO_MOCK_H