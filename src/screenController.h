#pragma once

#include "initFuntions.h"

extern struct tm timeinfo;

extern volatile int screenIterator;
extern const int iteratorIncreaser;

extern volatile int brightnessLevel;
extern const int brightnessLevelsTable[];
extern bool showBrightnessChangeFlag;
extern unsigned long brightnessDebugPrintMillis;

void checkAndUpdateIcon(int x, int y);
void ScreenBrightnessChange();
void ScreenController();