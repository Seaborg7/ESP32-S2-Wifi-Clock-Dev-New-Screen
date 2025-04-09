#pragma once

#include <Arduino.h>

struct WeatherDataStruct
{
    String time = "NA";        // const char *time = "NA";
    String description = "NA"; // const char *description = "NA";
    float temp = 0;            // Temperatura
    float feelsLike = 0;       // Temperatura odczuwalna
    float windSpeed = 0;       // Prędkość wiatru
    float rain = 0;            // Deszcz w ciągu 3 godzin (domyślnie 0.0)
    float snow = 0;            // Śnieg w ciągu 3 godzin (domyślnie 0.0)
    float humidity = 0;        // Wilgotność
    String icon = "NA";        // Ikona pogody
};

extern WeatherDataStruct weatherData[];

void fetchCurrentWeather();
void fetchWeatherForecast();
void getAllWeatherData();