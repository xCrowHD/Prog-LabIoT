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

    THrandomGenerator generator(tDew, hNoise, tNoise, theta);

    // Parametri ambientali
    float avgTemp = 19.0f;
    float excursion = 14.0f;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "--- INIZIO TEST SIMULAZIONE 24 ORE ---" << std::endl;
    std::cout << "Ora\tTemp (C)\tUmidita' (%)\tNota" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;

    // Simuliamo un campionamento ogni ora per un giorno intero
    for (int hour = 0; hour <= 24; ++hour)
    {
        float currentTime = static_cast<float>(hour);

        // Estraiamo il campione
        WeatherData data = generator.getSample(currentTime, avgTemp, excursion);

        // Aggiungiamo una nota per i punti salienti
        std::string note = "";
        if (hour == 2)
            note = "<-- Minimo Termico Atteso";
        if (hour == 14)
            note = "<-- Massimo Termico Atteso";
        if (hour == 20)
            note = "<-- Raffreddamento Serale";

        std::cout << hour << ":00\t"
                  << data.temperature << "\t\t"
                  << data.humidity << "\t\t"
                  << note << std::endl;
    }

    std::cout << "----------------------------------------------------" << std::endl;

    // --- TEST DI REATTIVITÀ ---
    std::cout << "\n--- TEST REATTIVITA' (Inerzia con theta) ---" << std::endl;
    std::cout << "Simuliamo 5 campionamenti rapidi nello stesso istante (ore 14:00)" << std::endl;
    std::cout << "Sconsigliato tenere il primo campione perche' e' influenzato solo dal rumore, ma serve per inizializzare il drift." << std::endl;

    for (int i = 0; i < 10; ++i)
    {
        // Il tempo non avanza, ma il rumore e il drift agiscono
        WeatherData rapidData = generator.getSample(14.0f, avgTemp, excursion);
        std::cout << "Lettura " << i + 1 << ": T = " << rapidData.temperature << " C" << std::endl;
    }

    return 0;
}