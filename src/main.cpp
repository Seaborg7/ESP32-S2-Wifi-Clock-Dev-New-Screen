// z jakiegoś powodu kod ten przestał działać przez TFT_eSPI

// TODO
// 1# DONE
// std::string trzeba by było zamienić na String (zgodnie z tym co mowi copilot)
// Rekomendacja
//         Dla ESP32/Arduino lepiej używać String
//         std::string tylko jeśli kod musi być kompatybilny ze standardowym C++
// 2# DONE
// fix 2giej linijki ekranu, gdzie 02-01 Czwartek zamieniło się na 003-01 Piatekk
//
// 3# DONE
// Sprawdzić jak działa większa czcionka dla real temp i feel temp
//
// 4#
// Wprowadzić ikony pogody
//
// 5#
// Pomyśleć o możliwości wyłączenia ekranu
//
// 6#
// Podpiąć czujnik wilgotności i temperatury
//
// 7#
// Druk obudowy

#include "initFuntions.h"

#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <PNGdec.h>

#include <Arduino.h>

#include <time.h>
#include "DHT.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <TimeLib.h>

// Comment out to stop drawing red spots
#define RED_SPOT

#define DHT_PIN 39
#define DHTTYPE DHT11
#define LED_PIN 15

#define MAX_IMAGE_WIDTH 240 // Adjust for your images

PNG png;
File pngFile;

// Zmienne do przechowywania pozycji obrazu
int imageX = 0; // Pozycja X obrazu
int imageY = 0; // Pozycja Y obrazu

volatile bool buttonState = false;
volatile bool prevButtonState = false;
volatile bool ledState = false;
volatile bool readSensorFlag = true; // Flaga do odczytu lokalnego czujnika

hw_timer_t *timerScreenIter = NULL;
hw_timer_t *timerLocalSensor = NULL;
uint16_t touchX, touchY;

volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 300;
const unsigned long holdTime = 10000;

const char *api_key = "002d66f5685c8adf7b26d312dea451a6";
const char *lat = "51.7829692";
const char *lon = "19.458378";

volatile float humidity = 80;
volatile float temperature = 24;

struct tm timeinfo;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // Dostosuj do swojej strefy czasowej
const int daylightOffset_sec = 3600;
const char *daysOfWeek[] = {
    "Ndz",
    "Pon",
    "Wt ",
    "Sr ",
    "Czw",
    "Pt ",
    "Sob"};
// const char *daysOfWeek[] = {"Ndz", "Pon", "Wt", "Sr", "Czw", "Pt", "Sob"};

volatile int screenIterator = 0;
volatile unsigned long screenDelay = 5000;
volatile unsigned long screenCurrentMillis = millis();
volatile unsigned long screenLastCallTime = 0;

unsigned long task1Delay = 3000;
unsigned long task2Delay = 10000;

const int iteratorIncreaser = 0;

struct DelayedCall
{
    unsigned long lastCallTime;
    unsigned long delay;
    std::function<void()> func;
};

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
} weatherData[40];
struct Image
{
    String name;
    uint8_t *data;
    size_t size;
};

void *pngOpen(const char *filename, int32_t *size)
{
    // Serial.printf("Opening %s\n", filename);
    pngFile = SPIFFS.open(filename, "rb");
    *size = pngFile.size();
    return &pngFile;
}

void pngClose(void *handle)
{
    if (pngFile)
        pngFile.close();
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
{
    return pngFile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position)
{
    return pngFile.seek(position);
}

void pngDraw(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0x00000000); // Czarny jako przezroczysty

    for (int i = 0; i < pDraw->iWidth; i++)
    {
        uint16_t color = lineBuffer[i];
        if (color != 0x0000) // Pomijamy czarne piksele (przezroczyste)
        {
            tft.drawPixel(imageX + i, imageY + pDraw->y, color);
        }
    }
}

void displayPNG(const char *filename, int x, int y)
{
    imageX = x;
    imageY = y;

    int rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (rc == PNG_SUCCESS)
    {
        // Serial.printf("Displaying image at (%d, %d)\n", x, y);
        // Serial.printf("Image size: %d x %d\n", png.getWidth(), png.getHeight());

        uint32_t start = millis();
        rc = png.decode(NULL, 0);
        // Serial.printf("Decode time: %d ms\n", millis() - start);

        png.close();
    }
    else
    {
        Serial.printf("PNG error: %d\n", rc);
    }
}

void callAfterDelay(DelayedCall &task)
{
    unsigned long currentMillis = millis();
    if (currentMillis - task.lastCallTime >= task.delay)
    {
        task.func();
        task.lastCallTime = currentMillis;
    }
}

DHT dht(DHT_PIN, DHTTYPE);

void getLocalSensorMeasurements()
{
    // humidity = dht.readHumidity();
    // temperature = dht.readTemperature();
    // // float hic = dht.computeHeatIndex(temperature, humidity, false);

    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();

    if (!isnan(newHumidity) && !isnan(newTemperature) && newHumidity > 0 && newTemperature > 0)
    {
        humidity = newHumidity;
        temperature = newTemperature;
        Serial.printf("Odczyt: Temperatura: %.1f *C  Wilgotność: %.0f %%\n", temperature, humidity);
    }
    else
    {
        // Serial.println("Błąd odczytu z czujnika DHT11!");
        Serial.printf("Odczyt: Błąd odczytu z czujnika DHT11! Temperatura: %.1f *C  Wilgotność: %.0f %%\n", newTemperature, newHumidity);
    }
}

String formatDateTime(const String &dateTime)
{
    if (screenIterator == 0)
    {
        // Serial.printf("formatDateTime:%s", dateTime.c_str());
        return dateTime;
    }
    else
    {
        String day = dateTime.substring(8, 10);
        String month = dateTime.substring(5, 7);
        String time = dateTime.substring(11, 16);

        // Serial.printf("formatDateTime:%s||day:%s||month:%S||time:%s\n", dateTime.c_str(), day.c_str(), month.c_str(), time.c_str());

        return day + "." + month + " " + time;
    }
}

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

void printAllTnternalFileList()
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        Serial.print("FILE: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}

void drawCenteredText(const String &text, int y, int x = -1)
{
    tft.setTextDatum(TC_DATUM);
    if (x == -1)
    {
        x = tft.width() / 2;
    }
    tft.drawString(text, x, y);
}

void checkAndUpdateIcon(int x, int y)
{
    static String previousIcon = "";
    String currentIcon = weatherData[screenIterator + iteratorIncreaser].icon;
    if (currentIcon != previousIcon)
    {
        tft.fillRect(10, 130, 100, 100, TFT_BLACK); // Wyczyść obszar, rysując czarny kwadrat
        displayPNG(("/" + currentIcon + ".png").c_str(), x, y);
        previousIcon = currentIcon;
    }
}

void checkAndUpdateTempAndHumidity(char *buffer)
{
    static float previousTemperature = 0;
    static float previousHumidity = 0;

    if (temperature != previousTemperature || humidity != previousHumidity)
    {
        tft.fillRect(0, (8 * 5 + 2) + (8 * 2 + 2), tft.width(), 8 * 3, TFT_BLACK); // Wyczyść obszar, rysując czarny kwadrat

        tft.setTextSize(3);
        sprintf(buffer, "%.1f'C  %.0f%%", temperature, humidity);
        drawCenteredText(String(buffer), (8 * 5 + 2) + (8 * 2 + 2));

        previousTemperature = temperature;
        previousHumidity = humidity;
    }
}

void ScreenController()
{
    // Font Height is 8 pixels for size 1, 16 for size 2 etc, thats why HEIGHT is multiplied by 8 + 2 for spacing
    char buffer[50];

    tft.setTextColor(TFT_WHITE, TFT_BLACK, true);

    // 1st line - time
    tft.setTextSize(5);
    sprintf(buffer, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    drawCenteredText(String(buffer), 0);

    // 2nd line - date
    tft.setTextSize(2);
    sprintf(buffer, "%02d-%02d %s", timeinfo.tm_mday, timeinfo.tm_mon + 1, daysOfWeek[timeinfo.tm_wday]);
    drawCenteredText(String(buffer), (8 * 5 + 2));

    // tft.setCursor(25, (8 * 5 + 2) + (8 * 2 + 2));
    // tft.setTextSize(3);
    // tft.printf("%.1f'C", temperature);
    // tft.setCursor(160, (8 * 5 + 2) + (8 * 2 + 2));
    // tft.printf("%.0f%% ", humidity);

    // 3rd line - temperature and humidity
    // tft.setTextSize(3);
    // sprintf(buffer, "%.1f'C  %.0f%%", temperature, humidity);
    // drawCenteredText(String(buffer), (8 * 5 + 2) + (8 * 2 + 2));
    checkAndUpdateTempAndHumidity(buffer);

    // tft.setCursor(60, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2));
    // tft.setTextSize(2);
    // tft.printf("%s  %s", weatherData[0].time.substr(0, 4).c_str(), weatherData[0].time.substr(5, 10).c_str());

    // 4th line - sunrise and sunset
    tft.setTextSize(2);
    sprintf(buffer, "%s  %s", weatherData[0].time.substring(0, 5).c_str(), weatherData[0].time.substring(6, 11).c_str());
    drawCenteredText(String(buffer), (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2));

    // tft.drawFastHLine(200, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2), 210, TFT_WHITE);
    tft.drawLine(20, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2), 220, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2), TFT_WHITE);
    //-----------------------------------------------------------------------------------------------------------------------//  LINE on 104-105
    if (screenIterator > 0)
    {
        tft.setTextSize(2);
        sprintf(buffer, "%s", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str());
        drawCenteredText(String(buffer), 108);
    }
    else
    {
        tft.setTextSize(2);
        sprintf(buffer, "%s", "           ");
        drawCenteredText(String(buffer), 108);
    }

    tft.setTextSize(1);
    tft.setCursor(130, 108 + (8 * 2 + 4));
    tft.printf("%s", "temp. rzeczywista");

    tft.setTextSize(3);
    sprintf(buffer, "%5.1f'C", weatherData[screenIterator + iteratorIncreaser].temp); // max 5 znaków szerokości
    tft.setTextDatum(TR_DATUM);                                                       // TR_DATUM
    tft.drawString(String(buffer), 240, 108 + (8 * 2 + 4) + (8 * 1 + 2));

    tft.setTextSize(1);
    tft.setCursor(130, 108 + (8 * 2 + 4) + (8 * 1 + 2) + (8 * 3 + 10));
    tft.printf("%s", "temp. odczuwalna");

    tft.setTextSize(3);
    sprintf(buffer, "%5.1f'C", weatherData[screenIterator + iteratorIncreaser].feelsLike); // max 5 znaków szerokości
    tft.setTextDatum(TR_DATUM);
    tft.drawString(String(buffer), 240, 108 + (8 * 2 + 4) + (8 * 1 + 2) + (8 * 3 + 10) + (8 * 1 + 2));

    // tft.drawRect(10, 130, 100, 100, TFT_WHITE); // 10, 130, 110, 230
    // displayPNG(("/" + weatherData[screenIterator + iteratorIncreaser].icon + ".png").c_str(), 10, 110); // edit pozycji
    checkAndUpdateIcon(10, 110);

    tft.setTextSize(2);
    tft.setCursor(15, 235);
    tft.printf("Wiatr: %.0f km/h\n", weatherData[screenIterator + iteratorIncreaser].windSpeed * 3.6f);
    tft.setCursor(15, 253);
    tft.printf("Deszcz: %.1f mm\n", weatherData[screenIterator + iteratorIncreaser].rain);
    tft.setCursor(15, 271);
    tft.printf("Snieg: %.1f mm\n", weatherData[screenIterator + iteratorIncreaser].snow);

    // // Small iterable text on bottom half of the screen //
    // tft.setCursor(0, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2) + 100);
    // tft.setTextSize(2);
    // tft.printf("%s %.1f %.1f\n", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str(), weatherData[screenIterator + iteratorIncreaser].temp, weatherData[screenIterator + iteratorIncreaser].feelsLike);
    // tft.printf("%s\n", weatherData[screenIterator + iteratorIncreaser].description.c_str());
    // tft.printf("%.0f km/h %.1f %.1f mm\n", weatherData[screenIterator + iteratorIncreaser].windSpeed * 3.6f, weatherData[screenIterator + iteratorIncreaser].rain, weatherData[screenIterator + iteratorIncreaser].snow);

    // tft.printf("%s %s\n", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str(), weatherData[screenIterator + iteratorIncreaser].description.c_str());
    // tft.printf("Temp: %.1f 'C Feels like: %.1f 'C\n", weatherData[screenIterator + iteratorIncreaser].temp, weatherData[screenIterator + iteratorIncreaser].feelsLike);
    // tft.printf("Wind %.2f m/s Rain(3h) %.2f mm\n", weatherData[screenIterator + iteratorIncreaser].windSpeed, weatherData[screenIterator + iteratorIncreaser].rain);
    Serial.printf("Time: %s   ScreenIter: %d\n", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str(), screenIterator);
    Serial.printf("Description: %s \n", weatherData[screenIterator + iteratorIncreaser].description.c_str());
    Serial.printf("Temp: %.1f 'C Feels like: %.1f 'C\n", weatherData[screenIterator + iteratorIncreaser].temp, weatherData[screenIterator + iteratorIncreaser].feelsLike);
    Serial.printf("Wind %.0f km/h Rain(3h) %.1f mm Snow(3h) %.1f mm\n", weatherData[screenIterator + iteratorIncreaser].windSpeed * 3.6f, weatherData[screenIterator + iteratorIncreaser].rain, weatherData[screenIterator + iteratorIncreaser].snow);
}

void IRAM_ATTR handleButtonPress()
{
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay)
    {
        if (tft.getTouch(&touchX, &touchY))
        {
            // Serial.println("Click, zaczynam nowy timerScreenIter ******************************************************");
            if (touchX > 120)
            {
                screenIterator = (screenIterator + 1) % 39;
            }
            else
            {
                if (screenIterator > 0)
                {
                    screenIterator = (screenIterator - 1) % 39;
                }
            }

            // ScreenController();

            // timerStop(timerScreenIter);
            timerWrite(timerScreenIter, 0);
            timerStart(timerScreenIter);

            // timerRestart(timerScreenIter);

            // timerWrite(timerScreenIter, 0);
            // timerAlarmEnable(timerScreenIter);

            digitalWrite(LED_PIN, true);
            lastDebounceTime = currentTime;
        }
    }
}

void IRAM_ATTR onTimerScreenIter()
{
    timerStop(timerScreenIter);
    // timerAlarmDisable(timerScreenIter);

    screenIterator = 0;
    digitalWrite(LED_PIN, false);
    // Serial.println("Stopuje timerScreenIter ******************************************************");
}

void IRAM_ATTR onTimerLocalSensor()
{
    readSensorFlag = true; // Ustaw flagę w przerwaniu
}

void drawDotsOnTouch()
{
#ifdef RED_SPOT
    uint16_t x, y;

    // See if there's any touch data for us
    if (tft.getTouch(&x, &y))
    {
        // Draw a block spot to show where touch was calculated to be
        tft.fillCircle(x, y, 2, TFT_RED);
    }
#endif
}

// DelayedCall task1 = {0, task1Delay, getLocalSensorMeasurements};
DelayedCall task2 = {0, task2Delay, getAllWeatherData};

//------------------------------------------------------------------------------------------
void setup(void)
{
    Serial.begin(9600);
    WifiInit();
    dht.begin();

    tft.init();
    tft.setRotation(2);
    touch_calibrate();
    tft.fillScreen(TFT_BLACK);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(LED_PIN, OUTPUT);
    // attachInterrupt(BUTTON_PIN, handleButtonPress, RISING);
    attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), handleButtonPress, FALLING);

    fetchCurrentWeather();
    fetchWeatherForecast();

    timerScreenIter = timerBegin(0, 80, true);                       // Timer 0, prescaler 80, licznik w trybie up-count
    timerAttachInterrupt(timerScreenIter, &onTimerScreenIter, true); // Ustawienie przerwania
    timerAlarmWrite(timerScreenIter, 5000000, true);                 // 5 sekund
    timerAlarmEnable(timerScreenIter);                               // Włączenie alarmu

    timerLocalSensor = timerBegin(1, 80, true);                        // Timer 1, prescaler 80, licznik w trybie up-count
    timerAttachInterrupt(timerLocalSensor, &onTimerLocalSensor, true); // Ustawienie przerwania
    timerAlarmWrite(timerLocalSensor, 2000000, true);                  // 2 sekundy
    timerAlarmEnable(timerLocalSensor);                                // Włączenie alarmu

    // delay(3000);

    printAllTnternalFileList();
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void loop()
{
    drawDotsOnTouch(); // #ifdef RED_SPOT

    // callAfterDelay(task1);
    callAfterDelay(task2);

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        // return;
    }

    if (readSensorFlag)
    {
        readSensorFlag = false;
        getLocalSensorMeasurements();
    }

    Serial.println("------------------------------------------------------");
    Serial.printf("Aktualny czas: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    Serial.printf("Wilgotność: %.0f %% \n", humidity);
    Serial.printf("Temperatura: %.1f *C \n", temperature);
    Serial.println("---------------");

    ScreenController();

    // display.printf("%.1f'C %.1f'C %.0fms %.0fmm\n", weatherData[0].temp, weatherData[0].feelsLike, weatherData[0].windSpeed, weatherData[0].rain);

    // display.printf("Temperatura: %.2f °C\n", weatherData[0].temp);
    // display.printf("Temperatura odczuwalna: %.2f °C\n", weatherData[0].feelsLike);
    // display.printf("Prędkość wiatru: %.2f m/s\n", weatherData[0].windSpeed);
    // display.printf("Deszcz (1h): %.2f mm\n", weatherData[0].rain);

    // delay(970); // Odśwież co sekundę
}
//------------------------------------------------------------------------------------------