#pragma once

#include "main.h"
#include "handlingFuntions.h"
#include "initFuntions.h"
#include "screenController.h"

extern volatile bool screenPressFlag;    // Flag for handling screen press
extern volatile bool readSensorFlag;     // Flag for reading local sensor data
extern volatile bool buttonPressFlag;    // Flag for button press handling
extern volatile bool screenSleepingFlag; // Flag for screen sleeping mode

struct DelayedCall
{
    unsigned long lastCallTime;
    unsigned long delay;
    std::function<void()> func;
};
extern DelayedCall taskWeatherData;

void callAfterDelay(DelayedCall &task);
void IRAM_ATTR handleScreenPress();
void IRAM_ATTR onTimerScreenIter();
void IRAM_ATTR onTimerLocalSensor();
void IRAM_ATTR handleBrightnessButton();