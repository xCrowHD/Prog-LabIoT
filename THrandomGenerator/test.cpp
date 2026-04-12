#include <iostream>
#include <iomanip>
#include <vector>
#include "THrandomGenerator.h"

int main()
{
    float tDew = 2.0f;
    float hNoise = 0.2f;
    float tNoise = 0.2f;
    float theta = 0.5f; 

    THrandomGenerator oncePerHour(tDew, hNoise, tNoise, theta);
    THrandomGenerator twicePerHour(tDew, hNoise, tNoise, theta);
    THrandomGenerator onceEveryTwoHours(tDew, hNoise, tNoise, theta);

    // Parametri ambientali
    float avgTemp = 7.0f;
    float excursion = 6.0f;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "--- INIZIO TEST SIMULAZIONE 24 ORE ---" << std::endl;
    std::cout << "Ora\tTemp (C)\tUmidita' (%)\tNota" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;

    for (int step = 0; step <= 48; ++step)
    {
        // Il tempo deve avanzare di 0.5 ore a ogni step
        float currentTime = step * 0.5f;

        // Calcolo per la stampa (es. 1.5 ore -> 01:30)
        int h = static_cast<int>(currentTime);
        int m = (step % 2 == 0) ? 0 : 30;

        // 1. Mezz'ora (Sempre)
        WeatherData d2 = twicePerHour.getSample(currentTime, avgTemp, excursion);
        std::cout << std::setfill('0') << std::setw(2) << h << ":" << std::setw(2) << m
                  << "\t" << d2.temperature << "\t" << d2.humidity << "\t[Ogni 30 min]" << std::endl;

        // 2. Ogni Ora (Solo quando step è pari)
        if (step % 2 == 0)
        {
            WeatherData d1 = oncePerHour.getSample(currentTime, avgTemp, excursion);
            std::cout << std::setw(2) << h << ":00\t" << d1.temperature
                      << "\t" << d1.humidity << "\t[Ogni Ora]" << std::endl;
        }

        // 3. Ogni Due Ore (Solo quando step è multiplo di 4)
        if (step % 4 == 0)
        {
            WeatherData d05 = onceEveryTwoHours.getSample(currentTime, avgTemp, excursion);
            std::cout << std::setw(2) << h << ":00\t" << d05.temperature
                      << "\t" << d05.humidity << "\t[Ogni 2 Ore]" << std::endl;
        }
        std::cout << "----------------------------------------------------" << std::endl;
    }

    // --- TEST DI REATTIVITÀ ---
    std::cout << "\n--- TEST REATTIVITA' (Inerzia con theta) ---" << std::endl;
    std::cout << "Simuliamo 5 campionamenti rapidi nello stesso istante (ore 14:00)" << std::endl;
    std::cout << "Sconsigliato tenere il primo campione perche' e' influenzato solo dal rumore, ma serve per inizializzare il drift." << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        // Il tempo non avanza, ma il rumore e il drift agiscono
        WeatherData rapidData = oncePerHour.getSample(14.0f, avgTemp, excursion);
        std::cout << "Lettura " << i + 1 << ": T = " << rapidData.temperature << " C" << std::endl;
    }

    return 0;
}