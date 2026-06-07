#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <iostream>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <cmath>

#ifndef ARDUINO
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0

inline uint32_t _mock_millis_counter = 0;
inline unsigned long millis()
{
    return _mock_millis_counter;
}

inline void delay(unsigned long ms)
{
    _mock_millis_counter += ms;
}

inline void pinMode(uint8_t pin, uint8_t mode)
{
    std::cout << "[Mock in" << pin << "] set to " << mode << std::endl;
}
inline void digitalWrite(uint8_t pin, uint8_t val)
{
    (void)pin;
    (void)val;
}

inline int digitalRead(uint8_t pin)
{
    (void)pin;
    return HIGH; // Simula un pulsante non premuto
}

inline int analogRead(uint8_t pin)
{
    (void)pin;
    return 500;
}

// Risolve l'assenza di isnan globale su alcuni compilatori C++ standard
using std::isnan;
#endif

#define F(x) x

class SerialMock {
    public:
        void begin(int baudrate) {}

        void print(const char *str) { std::cout << str; }
        void print(char *str) { std::cout << str; }

        void println(const char *str) { std::cout << str << std::endl; }
        void println(char *str) { std::cout << str << std::endl; }

        template <typename T>
        void print(const T &val)
        {
            std::cout << val;
        }

        template <typename T>
        void println(const T &val)
        {
            std::cout << val << std::endl;
        }

        void println()
        {
            std::cout << std::endl;
        }

        void printf(const char *format, ...)
        {
            va_list args;
            va_start(args, format);
            std::vprintf(format, args);
            va_end(args);
        }

        template <typename T>
        size_t write(const T &val)
        {
            std::cout << val;
            return 1; // Arduino ritorna il numero di byte scritti
        }

        size_t write(const uint8_t *buffer, size_t size)
        {
            for (size_t i = 0; i < size; ++i)
            {
                // Cast a char per forzare la stampa del carattere visibile o del byte su terminale
                std::cout << static_cast<char>(buffer[i]);
            }
            return size;
        }

        void write(char * str, unsigned int size) {
            std::cout << str << std::endl;
        }
};

inline SerialMock Serial;
#endif // ARDUINO_MOCK_H