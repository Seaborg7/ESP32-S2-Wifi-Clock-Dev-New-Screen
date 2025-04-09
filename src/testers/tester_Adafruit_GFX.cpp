// #include <Adafruit_GFX.h>
// #include <Adafruit_ILI9341.h>
// #include <FS.h>
// #include <SPIFFS.h>

// #include <arduino.h>
// #include <WiFi.h>
// #include <time.h>
// #include "DHT.h"
// #include <HTTPClient.h>
// #include <ArduinoJson.h>
// #include <functional>
// #include <TimeLib.h>

// #define TFT_MISO 37 // -1
// #define TFT_MOSI 11
// #define TFT_SCLK 12
// #define TFT_CS 10
// #define TFT_DC 13
// #define TFT_RST 16
// #define TFT_BL 18

// // Definicje pinów dotyku (zmodyfikowane)
// #define TOUCH_CS 9    // T_CS
// #define TOUCH_IRQ 7   // T_IRQ
// #define TOUCH_MOSI 11 // T_DIN 38
// #define TOUCH_MISO 37 // T_DO
// #define TOUCH_CLK 12  // T_CLK 36

// // Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);

// void setup()
// {
//     tft.begin();
//     tft.setRotation(2);
//     tft.fillScreen(ILI9341_BLACK);
//     tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Tekst z tłem
// }

// void loop()
// {
//     static int counter = 1;

//     tft.setCursor(10, 10);
//     tft.setTextSize(2);
//     tft.printf("Counter: %d", counter, ILI9341_WHITE, ILI9341_BLACK);
//     tft.setCursor(10, 30);
//     // tft.fillRect(10, 30, 240, 40, ILI9341_BLACK); // Usuń stary tekst
//     tft.fillScreen(ILI9341_BLACK);
//     tft.printf("Counter: %d", counter);

//     // tft.drawChar(10, 10, counter, ILI9341_WHITE, ILI9341_BLACK, 2);
//     counter = (counter + 1);
//     if (counter >= 100)
//     {
//         counter = 1;
//     }
//     delay(50);

//     // tft.setCursor(10, 10);
//     // tft.fillRect(10, 10, 100, 20, ILI9341_BLACK); // Usuń stary tekst
//     // tft.print("Updated!");
//     // delay(10);
// }