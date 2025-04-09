#pragma once

#include "initFuntions.h"

extern struct tm timeinfo;

extern volatile int screenIterator;
extern const int iteratorIncreaser;

void checkAndUpdateIcon(int x, int y);
void ScreenController();