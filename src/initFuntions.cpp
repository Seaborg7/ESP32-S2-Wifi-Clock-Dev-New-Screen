#include "initFuntions.h"
#include <WiFi.h>
#include <SPIFFS.h>

// This is the file name used to store the touch coordinate
// calibration data. Change the name to start a new calibration.
#define CALIBRATION_FILE "/TouchCalData3"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

TFT_eSPI tft = TFT_eSPI();
DHT dht(DHT_PIN, DHTTYPE);
Preferences preferences; // Object for managing NVS memory

void WifiInit()
{
    const char *ssid = "TP-Link_Bocian";
    const char *password = "bocian01";

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(200);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
}

void TftInit()
{
    tft.init();
    tft.setRotation(2);
    touch_calibrate();
    tft.fillScreen(TFT_BLACK);

    // Backlight configuration
    pinMode(TFT_BL, OUTPUT);
    ledcAttachPin(TFT_BL, 0);                             // PWM channel is set to 0
    ledcSetup(0, 5000, 8);                                // Channel 0, frequency 5 kHz, resolution 8 bits
    brightnessLevel = loadBrightnessLevel("brightness");  // Load brightness level from NVS
    ledcWrite(0, brightnessLevelsTable[brightnessLevel]); // Set brightness based on NVS read
}

void touch_calibrate()
{
    uint16_t calData[5];
    uint8_t calDataOK = 0;

    // check file system exists
    if (!SPIFFS.begin())
    {
        Serial.println("Formatting file system");
        SPIFFS.format();
        SPIFFS.begin();
    }

    // check if calibration file exists and size is correct
    if (SPIFFS.exists(CALIBRATION_FILE))
    {
        if (REPEAT_CAL)
        {
            // Delete if we want to re-calibrate
            SPIFFS.remove(CALIBRATION_FILE);
        }
        else
        {
            File f = SPIFFS.open(CALIBRATION_FILE, "r");
            if (f)
            {
                if (f.readBytes((char *)calData, 14) == 14)
                    calDataOK = 1;
                f.close();
            }
        }
    }

    if (calDataOK && !REPEAT_CAL)
    {
        // calibration data valid
        tft.setTouch(calData);
    }
    else
    {
        // data not valid so recalibrate
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(20, 0);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        tft.println("Touch corners as indicated");

        tft.setTextFont(1);
        tft.println();

        if (REPEAT_CAL)
        {
            tft.setTextColor(TFT_RED, TFT_BLACK);
            tft.println("Set REPEAT_CAL to false to stop this running again!");
        }

        tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.println("Calibration complete!");

        File f = SPIFFS.open(CALIBRATION_FILE, "w");
        if (f)
        {
            f.write((const unsigned char *)calData, 14);
            f.close();
        }
    }
}

int loadBrightnessLevel(char const *name)
{
    preferences.begin("settings", true);     // Open the namespace "settings" in NVS for reading
    int level = preferences.getInt(name, 0); // Read the brightness level, default to 0 if not found
    preferences.end();                       // Close the namespace
    return level;
}