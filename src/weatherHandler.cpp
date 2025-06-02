#include "weatherHandler.h"

#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *api_key = "002d66f5685c8adf7b26d312dea451a6";
const char *lat = "51.7829692";
const char *lon = "19.458378";

WeatherDataStruct weatherData[40];

void fetchCurrentWeather()
{
    HTTPClient http;
    String url = String("http://api.openweathermap.org/data/2.5/weather?lat=") + lat +
                 "&lon=" + lon + "&units=metric&appid=" + api_key;
    // http://api.openweathermap.org/data/2.5/weather?lat=51.7829692&lon=19.458378&units=metric&appid=002d66f5685c8adf7b26d312dea451a6
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200)
    {
        String payload = http.getString();

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, payload);

        String timestamp = doc["dt"];

        time_t timezone = doc["timezone"];
        time_t sunrise = doc["sys"]["sunrise"];
        time_t sunset = doc["sys"]["sunset"];

        char sunriseStr[6], sunsetStr[6];
        strftime(sunriseStr, sizeof(sunriseStr), "%H:%M", localtime(&sunrise));
        strftime(sunsetStr, sizeof(sunsetStr), "%H:%M", localtime(&sunset));

        // weatherData[0].time = (String(hour(sunrise)) + ":" + String(minute(sunrise)) + " " + String(hour(sunset)) + ":" + String(minute(sunset))).c_str(); // Wschód i zachód słońca
        weatherData[0].time = String(sunriseStr) + " " + String(sunsetStr);         // Wschód i zachód słońca
        weatherData[0].description = doc["weather"][0]["description"].as<String>(); // Opis pogody
        weatherData[0].temp = doc["main"]["temp"];                                  // Temperatura
        weatherData[0].feelsLike = doc["main"]["feels_like"];                       // Temperatura odczuwalna
        weatherData[0].windSpeed = doc["wind"]["speed"];                            // Prędkość wiatru
        weatherData[0].rain = doc["rain"]["1h"] | 0.00;                             // Deszcz w ciągu 1 godziny (domyślnie 0.0)
        weatherData[0].snow = doc["snow"]["1h"] | 0.00;                             // Śnieg w ciągu 1 godziny (domyślnie 0.0)
        weatherData[0].humidity = doc["main"]["humidity"];                          // Wilgotność w procentach
        weatherData[0].icon = doc["weather"][0]["icon"].as<String>();               // Ikona pogody

        // Serial.printf("Wschód i zachód: %s \n", weatherData[0].time.c_str());
        // Serial.printf("Opis: %s \n", weatherData[0].description);
        // Serial.printf("Temperatura: %.2f °C\n", weatherData[0].temp);
        // Serial.printf("Temperatura odczuwalna: %.2f °C\n", weatherData[0].feelsLike);
        // Serial.printf("Prędkość wiatru: %.2f m/s\n", weatherData[0].windSpeed);
        // Serial.printf("Deszcz (1h): %.2f mm\n", weatherData[0].rain);
        // Serial.println("---------------------------");

        // Serial.println("---------local datas-------------");
        // Serial.printf("%s %.1f %.1f\n", formatDateTime(weatherData[0].time).c_str(), weatherData[0].temp, weatherData[0].feelsLike);
        // Serial.printf("%s\n", weatherData[0].description.c_str());
        // Serial.printf("%.0f km/h %.1f %.1f mm\n", weatherData[0].windSpeed * 3.6f, weatherData[0].rain, weatherData[0].snow);

        // Serial.printf("%s\n", weatherData[0].time.c_str());
        // Serial.printf("%.1f\n", weatherData[0].temp);
        // Serial.printf("%.1f\n", weatherData[0].feelsLike);
        // Serial.printf("%s\n", weatherData[0].description.c_str());
        // Serial.printf("%.0f km/h\n", weatherData[0].windSpeed * 3.6f);
        // Serial.printf("%.1f mm\n", weatherData[0].rain);
        // Serial.printf("%.1f mm\n", weatherData[0].snow);

        // Serial.println("-------------end-----------------");
    }
    else
    {
        Serial.printf("Błąd zapytania HTTP: %d\n", httpCode);
    }
    http.end();
}

void fetchWeatherForecast()
{
    HTTPClient http;
    String url = String("http://api.openweathermap.org/data/2.5/forecast?lat=") + lat +
                 "&lon=" + lon + "&units=metric&appid=" + api_key;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == 200)
    {
        String payload = http.getString();

        DynamicJsonDocument doc(20000);
        deserializeJson(doc, payload);

        JsonArray forecastList = doc["list"];
        for (int i = 0; i < 39; i++)
        {
            JsonObject forecast = forecastList[i];
            weatherData[i + 1].time = forecast["dt_txt"].as<String>();                           // Czas prognozy
            weatherData[i + 1].description = forecast["weather"][0]["description"].as<String>(); // Opis pogody
            weatherData[i + 1].temp = forecast["main"]["temp"];                                  // Temperatura
            weatherData[i + 1].feelsLike = forecast["main"]["feels_like"];                       // Temperatura odczuwalna
            weatherData[i + 1].windSpeed = forecast["wind"]["speed"];                            // Prędkość wiatru
            weatherData[i + 1].rain = forecast["rain"]["3h"] | 0.0;                              // Deszcz w ciągu 3 godzin (domyślnie 0.0)
            weatherData[i + 1].snow = forecast["snow"]["3h"] | 0.0;                              // Deszcz w ciągu 3 godzin (domyślnie 0.0)
            weatherData[i + 1].humidity = forecast["main"]["humidity"];                          // Wilgotność w procentach
            weatherData[i + 1].icon = forecast["weather"][0]["icon"].as<String>();               // Ikona pogody

            // Serial.println();
            // Serial.printf("Czas: %s\n", weatherData[i + 1].time.c_str());
            // Serial.printf("--Time: %s\n", formatDateTime(weatherData[i + 1].time).c_str());
            // Serial.printf("Czas: %s\n", weatherData[i + 1].time);
            // Serial.printf("Opis: %s\n", weatherData[i + 1].description);
            // Serial.printf("Temperatura: %.2f °C\n", weatherData[i + 1].temp);
            // Serial.printf("Temperatura odczuwalna: %.2f °C\n", weatherData[i + 1].feelsLike);
            // Serial.printf("Prędkość wiatru: %.2f m/s\n", weatherData[i + 1].windSpeed);
            // Serial.printf("Deszcz (3h): %.2f mm\n", weatherData[i + 1].rain);
            // Serial.println("------------------------------------------------------");
        }
    }
    else
    {
        Serial.printf("Błąd zapytania HTTP: %d\n", httpCode);
    }
    http.end();
}

void getAllWeatherData()
{
    fetchCurrentWeather();
    fetchWeatherForecast();
}
