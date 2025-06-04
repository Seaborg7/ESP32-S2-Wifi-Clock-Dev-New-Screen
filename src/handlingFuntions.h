#pragma once

#include "Arduino.h"
#include <PNGdec.h>
#include <SPIFFS.h>

extern PNG png;
extern File pngFile;

extern volatile float humidity;
extern volatile float temperature;

void drawCenteredText(const String &text, int y, int x = -1);
void getLocalSensorMeasurements();
void checkAndUpdateTempAndHumidity(char *buffer, int heightOffset);
String formatDateTime(const String &dateTime);
void *pngOpen(const char *filename, int32_t *size);
void pngClose(void *handle);
int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length);
int32_t pngSeek(PNGFILE *page, int32_t position);
void displayPNG(const char *filename, int x, int y);
void printAllTnternalFileList();
void saveSetting(int level, char const *name);