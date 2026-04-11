#include "THrandomGenerator.h"
#include <random>
#include <ctime>
#include <cmath>

THrandomGenerator::THrandomGenerator(float Tdew, float hNoisyness, float tNoisyness, float theta, bool firstSample)
    : Tdew(Tdew), hNoisyness(hNoisyness), tNoisyness(tNoisyness), theta(theta), firstSample(firstSample)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    generator.seed(seed);
    lastWeatherData = {0.0f, 0.0f};
}

WeatherData THrandomGenerator::getSample(float timeOftheDay, float avgTemperature, float dailyExcursion)
{
    float tClean = this->temperatureCurve(timeOftheDay, avgTemperature, dailyExcursion);
    
    float drift = 0.0f;
    if (firstSample)
    {
        firstSample = false;
        lastWeatherData.temperature = tClean; 
    }
    else
    {
        drift = this->theta * (tClean - lastWeatherData.temperature);
    }

    float tMeasured = this->applicateNoise(tClean + drift, this->tNoisyness);
    float hClean = this->MagnusTetens(tMeasured);
    float hMeasured = this->applicateNoise(hClean, this->hNoisyness);

    if (hMeasured > 100.0f)
        hMeasured = 100.0f;
    if (hMeasured < 0.0f)
        hMeasured = 0.0f;

    this->lastWeatherData.temperature = tClean;
    this->lastWeatherData.humidity = hClean;
    WeatherData data;
    data.temperature = tMeasured;
    data.humidity = hMeasured;
    
    return data;
}

float THrandomGenerator::temperatureCurve(float timeOftheDay, float avgTemperature, float dailyExcursion)
{
    float timeHottestHour = 14.0f;

    float temperatureValue = avgTemperature + dailyExcursion * std::sin(2.0f * M_PI * (timeOftheDay - (timeHottestHour - 6.0f)) / 24.0f);
    return temperatureValue;
}

float THrandomGenerator::applicateNoise(float cleanValue, float sigma)
{
    std::normal_distribution<float> noise(0.0f, sigma);
    return cleanValue + noise(generator);
}

float THrandomGenerator::MagnusTetens(float temperature)
{
    const float a = 17.625f; 
    const float b = 243.04f;
    
    float e_dry = std::exp((a * temperature) / (b + temperature));
    float e_dew = std::exp((a * this->Tdew) / (b + this->Tdew));
    return e_dew / e_dry * 100.0f;
}

