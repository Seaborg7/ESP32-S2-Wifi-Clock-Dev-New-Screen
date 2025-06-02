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
// 4# DONE
// Wprowadzić ikony pogody
//
// 5# DONE
// Pomyśleć o możliwości wyłączenia ekranu
//
// 6# DONE
// Podpiąć czujnik wilgotności i temperatury
//
// 7# DONE
// Druk obudowy

#include "initFuntions.h"
#include "screenController.h"
#include "handlingFuntions.h"
#include "weatherHandler.h"

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <functional>
#include <TimeLib.h>

// Comment out to stop drawing red spots
// #define RED_SPOT // RED_SPOT is causing to read "tft.getTouch" which leads to falling edge on pin T_IRQ

#define LED_PIN 15
#define BUTTON_PIN 0

volatile int brightnessLevel = 0;                                // Aktualny poziom jasności (indeks w tablicy)
const int brightnessLevelsTable[] = {255, 204, 153, 102, 51, 0}; // 100%, 80%, 60%, 40%, 20%, 0%
volatile bool buttonPressedFlag = false;                         // Flaga do obsługi przycisku

volatile bool buttonState = false;
volatile bool prevButtonState = false;
volatile bool ledState = false;
volatile bool readSensorFlag = true;   // Flag for reading local sensor data
volatile bool screenPressFlag = false; // Flag for handling screen press

hw_timer_t *timerScreenIter = NULL;
hw_timer_t *timerLocalSensor = NULL;
uint16_t touchX, touchY;

volatile unsigned long lastScreenDebounceTime = 0;
volatile unsigned long lastButtonDebounceTime = 0;
const unsigned long screenDebounceDelay = 300;
const unsigned long buttonDebounceDelay = 300;
const unsigned long holdTime = 10000;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // Adjust to your time zone
const int daylightOffset_sec = 3600;

volatile unsigned long timerScreenIterDelay = 5000000;  // 5 seconds
volatile unsigned long timerLocalSensorDelay = 2000000; // 2 seconds
// volatile unsigned long screenCurrentMillis = millis();
// volatile unsigned long screenLastCallTime = 0;

volatile unsigned long taskWeatherDataDelay = 10000;

struct DelayedCall
{
    unsigned long lastCallTime;
    unsigned long delay;
    std::function<void()> func;
};

struct Image
{
    String name;
    uint8_t *data;
    size_t size;
};

void callAfterDelay(DelayedCall &task)
{
    unsigned long currentMillis = millis();
    if (currentMillis - task.lastCallTime >= task.delay)
    {
        task.func();
        task.lastCallTime = currentMillis;
    }
}

void IRAM_ATTR handleScreenPress()
{
    unsigned long currentTime = millis();
    if (currentTime - lastScreenDebounceTime > screenDebounceDelay)
    {
        screenPressFlag = true;
        timerWrite(timerScreenIter, 0);
        timerStart(timerScreenIter);

        digitalWrite(LED_PIN, true);
        lastScreenDebounceTime = currentTime;
    }
}

void IRAM_ATTR onTimerScreenIter()
{
    timerStop(timerScreenIter);
    // timerAlarmDisable(timerScreenIter);

    screenIterator = 0;
    digitalWrite(LED_PIN, false);
    ledcWrite(0, brightnessLevelsTable[brightnessLevel]); // Ustaw jasność na podstawie odczytu z NVS
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

void IRAM_ATTR handleBrightnessButton()
{
    unsigned long currentTime = millis();
    if (currentTime - lastButtonDebounceTime > buttonDebounceDelay)
    {
        buttonPressedFlag = true;
        timerWrite(timerScreenIter, 0);
        timerStart(timerScreenIter);

        digitalWrite(LED_PIN, true);
        lastButtonDebounceTime = currentTime;
    }
}

DelayedCall taskWeatherData = {0, taskWeatherDataDelay, getAllWeatherData};

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

    // Konfiguracja podświetlenia
    pinMode(TFT_BL, OUTPUT);
    ledcAttachPin(TFT_BL, 0);                             // Kanał PWM 0
    ledcSetup(0, 5000, 8);                                // Kanał 0, częstotliwość 5 kHz, rozdzielczość 8 bitów
    brightnessLevel = loadBrightnessLevel();              // Odczytaj poziom jasności z NVS
    ledcWrite(0, brightnessLevelsTable[brightnessLevel]); // Ustaw jasność na podstawie odczytu z NVS

    // Konfiguracja przycisku
    pinMode(BUTTON_PIN, INPUT_PULLUP);                                                   // Przycisk z podciąganiem
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleBrightnessButton, FALLING); // Przerwanie na opadającym zboczu

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    pinMode(LED_PIN, OUTPUT);
    pinMode(TOUCH_IRQ, INPUT);
    attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), handleScreenPress, FALLING);

    fetchCurrentWeather();
    fetchWeatherForecast();

    timerScreenIter = timerBegin(0, 80, true);                       // Timer 0, prescaler 80, licznik w trybie up-count
    timerAttachInterrupt(timerScreenIter, &onTimerScreenIter, true); // Ustawienie przerwania
    timerAlarmWrite(timerScreenIter, timerScreenIterDelay, true);    // 5 sekund
    timerAlarmEnable(timerScreenIter);                               // Włączenie alarmu

    timerLocalSensor = timerBegin(1, 80, true);                        // Timer 1, prescaler 80, licznik w trybie up-count
    timerAttachInterrupt(timerLocalSensor, &onTimerLocalSensor, true); // Ustawienie przerwania
    timerAlarmWrite(timerLocalSensor, timerLocalSensorDelay, true);    // 2 sekundy
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
    callAfterDelay(taskWeatherData);

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

    if (screenPressFlag)
    {
        screenPressFlag = false;
        tft.getTouch(&touchX, &touchY);
        Serial.printf("New touch! X: %d, Y: %d\n", touchX, touchY);
        if (touchX > 120)
        {
            screenIterator = (screenIterator + 1) % 40;
        }
        else
        {
            if (screenIterator > 0)
            {
                screenIterator = (screenIterator - 1) % 40;
            }
            else if (screenIterator == 0)
            {
                screenIterator = 39;
            }
        }
        ledcWrite(0, brightnessLevelsTable[0]); // Max jasność
    }

    // Obsługa zmiany jasności
    if (buttonPressedFlag)
    {
        buttonPressedFlag = false;                            // Zresetuj flagę
        brightnessLevel = (brightnessLevel + 1) % 6;          // Przełącz na kolejny poziom jasności
        ledcWrite(0, brightnessLevelsTable[brightnessLevel]); // Ustaw nową jasność
        saveBrightnessLevel(brightnessLevel);                 // Zapisz jasność do NVS
        Serial.printf("Brightness level: %d%%\n", (brightnessLevelsTable[brightnessLevel] * 100) / 255);
    }

    ScreenController();

    // Serial.println("------------------------------------------------------");
    // Serial.printf("Aktualny czas: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    // Serial.printf("Wilgotność: %.0f %% \n", humidity);
    // Serial.printf("Temperatura: %.1f *C \n", temperature);
    // Serial.println("---------------");

    // display.printf("%.1f'C %.1f'C %.0fms %.0fmm\n", weatherData[0].temp, weatherData[0].feelsLike, weatherData[0].windSpeed, weatherData[0].rain);

    // display.printf("Temperatura: %.2f °C\n", weatherData[0].temp);
    // display.printf("Temperatura odczuwalna: %.2f °C\n", weatherData[0].feelsLike);
    // display.printf("Prędkość wiatru: %.2f m/s\n", weatherData[0].windSpeed);
    // display.printf("Deszcz (1h): %.2f mm\n", weatherData[0].rain);

    // delay(970); // Odśwież co sekundę
    // delay(500);
}
//------------------------------------------------------------------------------------------