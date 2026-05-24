#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <iostream>
#include <cstdint>

constexpr uint8_t LED_RED = 0;
constexpr uint8_t LED_GREEN = 1;
constexpr uint8_t LED_BLUE = 2;

constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;
constexpr uint8_t OUTPUT = 1;
constexpr uint8_t INPUT = 0;

inline uint8_t stateR = 0;
inline uint8_t stateG = 0;
inline uint8_t stateB = 0;

// Funzioni globali di Arduino (Mock)
void printCurrentColor();

inline void digitalWrite(uint8_t p, uint8_t value)
{
    // Aggiorna lo stato del pin specifico
    if (p == LED_RED)
        stateR = value;
    if (p == LED_GREEN)
        stateG = value;
    if (p == LED_BLUE)
        stateB = value;

    // Poiché setLedRGB fa 3 chiamate consecutive a digitalWrite,
    // stampiamo il colore finale solo quando l'ultimo pin (il Blu) viene aggiornato.
    if (p == LED_BLUE)
    {
        printCurrentColor();
    }
}

inline void pinMode(uint8_t p, uint8_t mode) {}

inline void printCurrentColor()
{
    if (stateR == HIGH && stateG == LOW && stateB == LOW)
        std::cout << "[MOCK LED] -> ROSSO\n";
    else if (stateR == LOW && stateG == HIGH && stateB == LOW)
        std::cout << "[MOCK LED] -> VERDE\n";
    else if (stateR == LOW && stateG == LOW && stateB == HIGH)
        std::cout << "[MOCK LED] -> BLU\n";
    else if (stateR == HIGH && stateG == HIGH && stateB == LOW)
        std::cout << "[MOCK LED] -> GIALLO\n";
    else if (stateR == HIGH && stateG == LOW && stateB == HIGH)
        std::cout << "[MOCK LED] -> VIOLA\n";
    else if (stateR == LOW && stateG == HIGH && stateB == HIGH)
        std::cout << "[MOCK LED] -> AZZURRO (Cyan)\n";
    else if (stateR == HIGH && stateG == HIGH && stateB == HIGH)
        std::cout << "[MOCK LED] -> BIANCO\n";
}

#endif // ARDUINO_MOCK_H