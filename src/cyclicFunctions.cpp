#include "cyclicFunctions.h"

volatile unsigned long lastScreenDebounceTime = 0;
volatile unsigned long lastButtonDebounceTime = 0;
const unsigned long screenDebounceDelay = 300;
const unsigned long buttonDebounceDelay = 400;

volatile bool screenPressFlag = false;    // Flag for handling screen press
volatile bool readSensorFlag = true;      // Flag for reading local sensor data
volatile bool buttonPressFlag = false;    // Flag for button press handling
volatile bool screenSleepingFlag = false; // Flag for screen sleeping mode

DelayedCall taskWeatherData = {0, taskWeatherDataDelay, getAllWeatherData};

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
}

void IRAM_ATTR onTimerLocalSensor()
{
    readSensorFlag = true;
}

void IRAM_ATTR handleBrightnessButton()
{
    unsigned long currentTime = millis();
    if (currentTime - lastButtonDebounceTime > buttonDebounceDelay)
    {
        buttonPressFlag = true;
        lastButtonDebounceTime = currentTime;
    }
}