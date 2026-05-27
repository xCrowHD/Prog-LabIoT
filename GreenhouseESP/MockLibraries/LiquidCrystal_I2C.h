#ifndef MOCK_LIQUID_CRYSTAL_I2C_H
#define MOCK_LIQUID_CRYSTAL_I2C_H

#include <stdio.h>

class LiquidCrystal_I2C {
    public:
        LiquidCrystal_I2C(int displayAddr, int displayChars, int displayLines) {
            std::cout << "LiquidCrystal_I2C constructor called, remember to call .begin()" << std::endl;
        }
        void begin(int displayChars, int displayLines) {
            std::cout << "LCD begin()" << std::endl;
        }
        void setBacklight(int backlight){
            std::cout << "Set LCD backlight to " << backlight << "/255" << std::endl; 
        }
        void clear() {
            std::cout << "LCD clear()" << std::endl;
        }
        void home() {
            std::cout << "LCD home()" << std::endl;
        }
        void print(const char* line) {
            std::cout << "LCD output: " << line << std::endl;
        }
        void setCursor(int row, int col) {}
};

#endif // MOCK_LIQUID_CRYSTAL_I2C_H