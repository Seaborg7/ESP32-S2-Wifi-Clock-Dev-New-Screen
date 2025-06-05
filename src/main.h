#pragma once

#include "initFuntions.h"
#include "screenController.h"
#include "handlingFuntions.h"
#include "weatherHandler.h"
#include "cyclicFunctions.h"

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#include <functional>
#include <TimeLib.h>

extern hw_timer_t *timerScreenIter;
extern hw_timer_t *timerLocalSensor;
extern uint16_t touchX, touchY;

extern volatile unsigned long taskWeatherDataDelay; // 60 seconds

struct Image
{
    String name;
    uint8_t *data;
    size_t size;
};