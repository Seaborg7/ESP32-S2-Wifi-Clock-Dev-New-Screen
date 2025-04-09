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
#include "screenController.h"
#include "handlingFuntions.h"
#include "weatherHandler.h"

#include <TFT_eSPI.h>

#include <Arduino.h>

#include <time.h>

#include <functional>
#include <TimeLib.h>

// Comment out to stop drawing red spots
#define RED_SPOT

#define LED_PIN 15

volatile bool buttonState = false;
volatile bool prevButtonState = false;
volatile bool ledState = false;
volatile bool readSensorFlag = true; // Flag for reading local sensor data

hw_timer_t *timerScreenIter = NULL;
hw_timer_t *timerLocalSensor = NULL;
uint16_t touchX, touchY;

volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 300;
const unsigned long holdTime = 10000;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // Adjust to your time zone
const int daylightOffset_sec = 3600;

volatile unsigned long screenDelay = 5000;
volatile unsigned long screenCurrentMillis = millis();
volatile unsigned long screenLastCallTime = 0;

unsigned long task1Delay = 3000;
unsigned long task2Delay = 10000;

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