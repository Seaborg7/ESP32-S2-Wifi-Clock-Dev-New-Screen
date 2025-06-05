#pragma once

#include <TFT_eSPI.h>
#include "DHT.h"
#include <Preferences.h>
#include "screenController.h"

#define DHT_PIN 39
#define DHTTYPE DHT11

extern TFT_eSPI tft;
extern DHT dht;
extern Preferences preferences; // Object for managing NVS memory

void WifiInit();
void TftInit();
void touch_calibrate();
int loadBrightnessLevel(char const *name);
