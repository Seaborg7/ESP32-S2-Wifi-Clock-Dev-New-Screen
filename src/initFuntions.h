#pragma once

#include <TFT_eSPI.h>
#include "DHT.h"

#define DHT_PIN 39
#define DHTTYPE DHT11

extern TFT_eSPI tft;
extern DHT dht;

void WifiInit();
void touch_calibrate();
