#pragma once

#include "initFuntions.h"
#include "handlingFuntions.h"
#include "weatherHandler.h"

extern struct tm timeinfo;

extern volatile int screenIterator;
extern const int iteratorIncreaser;

extern volatile int brightnessLevel;
extern const int brightnessLevelsTable[];
extern bool showBrightnessChangeFlag;
extern unsigned long brightnessDebugPrintMillis;
extern volatile bool screenSleepingFlag;

void checkAndUpdateIcon(int x, int y);
void screenBrightnessChange();
unsigned long realMillisInCurrent5sBlock();
void drawSecondProgressBar();
void screenSaver();
void screenController();