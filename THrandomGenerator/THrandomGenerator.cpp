#include "THrandomGenerator.h"
#include <random>
#include <ctime>
#include <cmath>

THrandomGenerator::THrandomGenerator(float Tdew, float hNoisyness, float tNoisyness, float theta, bool firstSample)
    : Tdew(Tdew), hNoisyness(hNoisyness), tNoisyness(tNoisyness), theta(theta), firstSample(firstSample)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    generator.seed(seed);
    lastWeatherData = {0.0f, 0.0f, 0.0f};
}

WeatherData THrandomGenerator::getSample(float timeOftheDay, float avgTemperature, float dailyExcursion)
{
    float tClean = this->temperatureCurve(timeOftheDay, avgTemperature, dailyExcursion);
    float tMeasured;

    if (firstSample)
    {
        firstSample = false;
        tMeasured = this->applicateNoise(tClean, this->tNoisyness);
    }
    else
    {
        float timeDiff = timeOftheDay - lastWeatherData.time;
        if (timeDiff < 0)
            timeDiff += 24.0f;

        if (timeDiff <= 0.0f) //letture simultanee, nessuna evoluzione, solo rumore
        {
            // introduco meno rumore per simulare lettura simultanea, 
            //altrimenti sarebbe troppo influenzata dal rumore
            tMeasured = this->applicateNoise(lastWeatherData.temperature, this->tNoisyness/5.0f); 
        }
        else
        {
            float alpha = std::exp(-this->theta * timeDiff);
            float tTarget = (lastWeatherData.temperature * alpha) + (tClean * (1.0f - alpha));
            float theoryNoise = this->tNoisyness * std::sqrt((1.0f - 
                std::exp(-2.0f * this->theta * timeDiff)) / (2.0f * this->theta));

            tMeasured = this->applicateNoise(tTarget, theoryNoise);
        }
    }
    float hClean = this->MagnusTetens(tMeasured);
    float hMeasured = this->applicateNoise(hClean, this->hNoisyness);

    if (hMeasured > 100.0f)
        hMeasured = 100.0f;
    else if (hMeasured < 0.0f)
        hMeasured = 0.0f;

    this->lastWeatherData.temperature = tMeasured;
    this->lastWeatherData.humidity = hMeasured;
    this->lastWeatherData.time = timeOftheDay;
    
    return this->lastWeatherData;
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

