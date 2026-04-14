#ifndef THrandomGenerator_h
#define THrandomGenerator_h

#include <random>

struct WeatherData
{
  float temperature;
  float humidity;
  float time;
  bool valid;
};

class THrandomGenerator{
  public:
    THrandomGenerator(float Tdew = 5.0f, float hNoisyness = 0.2f, float tNoisyness = 0.2f, float theta = 0.5f, bool firstSample = true);
    WeatherData getSample(float timeOftheDay, float avgTemperature, float dailyExcursion);

  private:
    std::default_random_engine generator;
    WeatherData lastWeatherData;
    int n_samples;
    float Tdew;
    float hNoisyness, tNoisyness, theta;
    bool firstSample = true;
    float temperatureCurve(float timeOftheDay, float avgTemperature, float dailyExcursion);
    float applicateNoise(float cleanValue, float sigma);
    float MagnusTetens(float temperature);
};

#endif