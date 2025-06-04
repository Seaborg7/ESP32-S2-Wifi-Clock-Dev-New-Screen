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

volatile bool screenSleepingFlag = false;
volatile int brightnessLevel = 0; // index in table brightnessLevelsTable
int lastBrightnessLevel = 0;
const int brightnessLevelsTable[] = {255, 204, 153, 102, 51, 0}; // 100%, 80%, 60%, 40%, 20%, 0%
volatile bool buttonPressedFlag = false;                         // Flag for button press handling

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
        lastScreenDebounceTime = currentTime;
    }
}

void IRAM_ATTR onTimerScreenIter()
{
    timerStop(timerScreenIter);
    // timerAlarmDisable(timerScreenIter);

    screenIterator = 0;
    digitalWrite(LED_PIN, false);
    ledcWrite(0, brightnessLevelsTable[brightnessLevel]); // Set brightness based on brightnessLevel value
    // Serial.println("Stopping timerScreenIter ******************************************************");
}

void IRAM_ATTR onTimerLocalSensor()
{
    readSensorFlag = true;
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

    // Backlight configuration
    pinMode(TFT_BL, OUTPUT);
    ledcAttachPin(TFT_BL, 0);                             // PWM channel is set to 0
    ledcSetup(0, 5000, 8);                                // Channel 0, frequency 5 kHz, resolution 8 bits
    brightnessLevel = loadBrightnessLevel("brightness");  // Load brightness level from NVS
    ledcWrite(0, brightnessLevelsTable[brightnessLevel]); // Set brightness based on NVS read

    // Button configuration
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleBrightnessButton, FALLING); // Set up interrupt for button press on falling edge

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    pinMode(LED_PIN, OUTPUT);
    pinMode(TOUCH_IRQ, INPUT);
    attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), handleScreenPress, FALLING);

    fetchCurrentWeather();
    fetchWeatherForecast();

    timerScreenIter = timerBegin(0, 80, true);                       // Timer 0, prescaler 80, counter in up-count mdoe
    timerAttachInterrupt(timerScreenIter, &onTimerScreenIter, true); // Setup interrupt
    timerAlarmWrite(timerScreenIter, timerScreenIterDelay, true);    // 5 seconds
    timerAlarmEnable(timerScreenIter);                               // Alarm enable

    timerLocalSensor = timerBegin(1, 80, true);                        // Timer 1, prescaler 80, counter in up-count mdoe
    timerAttachInterrupt(timerLocalSensor, &onTimerLocalSensor, true); // Setup interrupt
    timerAlarmWrite(timerLocalSensor, timerLocalSensorDelay, true);    // 2 seconds
    timerAlarmEnable(timerLocalSensor);                                // Alarm enable

    printAllTnternalFileList();
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void loop()
{
    drawDotsOnTouch(); // #ifdef RED_SPOT

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
        timerWrite(timerScreenIter, 0);
        timerStart(timerScreenIter);
        digitalWrite(LED_PIN, true);
        tft.getTouch(&touchX, &touchY);
        // Serial.printf("New touch! X: %d, Y: %d\n", touchX, touchY);

        if (screenSleepingFlag)
        {
            screenSleepingFlag = false;
            brightnessLevel = loadBrightnessLevel("brightnessLast"); // Load last brightness level from NVS
            saveSetting(brightnessLevel, "brightness");              // Save current brightness level to NVS
            ledcWrite(0, brightnessLevelsTable[brightnessLevel]);    // Restore brightness
        }
        else
        {
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
            ledcWrite(0, brightnessLevelsTable[0]); // Set brightness to max
        }
    }

    // Brightness level changes maintenance
    if (buttonPressedFlag)
    {
        buttonPressedFlag = false;
        brightnessLevel = (brightnessLevel + 1) % 6;
        ledcWrite(0, brightnessLevelsTable[brightnessLevel]);
        saveSetting(brightnessLevel, "brightness");
    }

    if ((timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0) ||
        (timeinfo.tm_hour == 2 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0) ||
        (timeinfo.tm_hour == 4 && timeinfo.tm_min == 0 && timeinfo.tm_sec == 0))
    {
        if (brightnessLevel > 0)
        {
            saveSetting(brightnessLevel, "brightnessLast"); // Store last brightness level for future unsleep
            brightnessLevel = 0;
            saveSetting(brightnessLevel, "brightness");
            ledcWrite(0, brightnessLevelsTable[brightnessLevel]);
            screenSleepingFlag = true;
        }
    }

    ScreenController();
}
//------------------------------------------------------------------------------------------