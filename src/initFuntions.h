#pragma once

#include <TFT_eSPI.h>
#include "DHT.h"
#include <Preferences.h>

#define DHT_PIN 39
#define DHTTYPE DHT11

extern TFT_eSPI tft;
extern DHT dht;
extern Preferences preferences; // Obiekt do zarządzania pamięcią NVS

void WifiInit();
void touch_calibrate();
int loadBrightnessLevel();
