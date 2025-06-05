#include "main.h"
#include "handlingFuntions.h"
#include "initFuntions.h"
#include "screenController.h"
#include "weatherHandler.h"

PNG png;
File pngFile;

int imageX = 0;
int imageY = 0;

volatile float humidity = 80;
volatile float temperature = 20;

#define MAX_IMAGE_WIDTH 240

void drawCenteredText(const String &text, int y, int x)
{
    tft.setTextDatum(TC_DATUM);
    if (x == -1)
    {
        x = tft.width() / 2;
    }
    tft.drawString(text, x, y);
}

void getLocalSensorMeasurements()
{
    // humidity = dht.readHumidity();
    // temperature = dht.readTemperature();
    // // float hic = dht.computeHeatIndex(temperature, humidity, false);

    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();

    if (!isnan(newHumidity) && !isnan(newTemperature) && newHumidity > 0 && newTemperature > 0)
    {
        humidity = newHumidity;
        temperature = newTemperature;
        // Serial.printf("Sensor reads: Temperature: %.1f *C  Humidity: %.0f %%\n", temperature, humidity);
    }
    else
    {
        Serial.printf("Sensor reads: Fail at read sensor DHT11! Temperature: %.1f *C  Humidity: %.0f %%\n", newTemperature, newHumidity);
    }
}

void checkAndUpdateTempAndHumidity(char *buffer, int heightOffset)
{
    static float previousTemperature = 0;
    static float previousHumidity = 0;

    if (temperature != previousTemperature || humidity != previousHumidity)
    {
        tft.fillRect(0, heightOffset + (8 * 5 + 2) + (8 * 2 + 2), tft.width(), 8 * 3, TFT_BLACK); // Clear the area for temperature and humidity

        tft.setTextSize(3);
        sprintf(buffer, "%.1f'C  %.0f%%", temperature, humidity);
        drawCenteredText(String(buffer), heightOffset + (8 * 5 + 2) + (8 * 2 + 2));

        previousTemperature = temperature;
        previousHumidity = humidity;
    }
}

String formatDateTime(const String &dateTime)
{
    if (screenIterator == 0)
    {
        // Serial.printf("formatDateTime:%s", dateTime.c_str());
        return dateTime;
    }
    else
    {
        String day = dateTime.substring(8, 10);
        String month = dateTime.substring(5, 7);
        String time = dateTime.substring(11, 16);

        // Serial.printf("formatDateTime:%s||day:%s||month:%S||time:%s\n", dateTime.c_str(), day.c_str(), month.c_str(), time.c_str());

        return day + "." + month + " " + time;
    }
}

void *pngOpen(const char *filename, int32_t *size)
{
    // Serial.printf("Opening %s\n", filename);
    pngFile = SPIFFS.open(filename, "rb");
    *size = pngFile.size();
    return &pngFile;
}

void pngClose(void *handle)
{
    if (pngFile)
        pngFile.close();
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
{
    return pngFile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position)
{
    return pngFile.seek(position);
}

void pngDraw(PNGDRAW *pDraw)
{
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0x00000000); // Black as transparent

    for (int i = 0; i < pDraw->iWidth; i++)
    {
        uint16_t color = lineBuffer[i];
        if (color != 0x0000) // Skips transparent pixels
        {
            tft.drawPixel(imageX + i, imageY + pDraw->y, color);
        }
    }
}

void displayPNG(const char *filename, int x, int y)
{
    imageX = x;
    imageY = y;

    int rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
    if (rc == PNG_SUCCESS)
    {
        // Serial.printf("Displaying image at (%d, %d)\n", x, y);
        // Serial.printf("Image size: %d x %d\n", png.getWidth(), png.getHeight());

        uint32_t start = millis();
        rc = png.decode(NULL, 0);
        // Serial.printf("Decode time: %d ms\n", millis() - start);

        png.close();
    }
    else
    {
        Serial.printf("PNG error: %d\n", rc);
    }
}

void printAllTnternalFileList()
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        Serial.print("FILE: ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
}

void saveSetting(int level, const char *name)
{
    preferences.begin("settings", false); // Open the namespace "settings" in NVS
    preferences.putInt(name, level);      // Save "name" value
    preferences.end();                    // Close namespace
}

void localTimeHandler(struct tm *timeinfo)
{
    if (!getLocalTime(timeinfo))
    {
        Serial.println("Failed to obtain time");
        // return;
    }
}

void localSensorMeasurementsHandler(bool *readSensorFlag)
{
    if (*readSensorFlag)
    {
        *readSensorFlag = false;
        getLocalSensorMeasurements();
    }
}
void screenPressHandler(volatile bool *screenPressFlag)
{
    if (*screenPressFlag)
    {
        *screenPressFlag = false;
        timerWrite(timerScreenIter, 0);
        timerStart(timerScreenIter);
        digitalWrite(LED_PIN, true);
        tft.getTouch(&touchX, &touchY);
        // Serial.printf("New touch! X: %d, Y: %d\n", touchX, touchY);

        if (screenSleepingFlag)
        {
            screenSleepingFlag = false;
            brightnessLevel = loadBrightnessLevel("brightnessLast"); // Load last brightness level from NVS
            saveSetting(brightnessLevel, "brightness");              // Save current brightness level to NVS
            ledcWrite(0, brightnessLevelsTable[brightnessLevel]);    // Restore brightness
        }
        else
        {
            if (touchX > 120)
            {
                screenIterator = (screenIterator + 1) % 40;
            }
            else
            {
                if (screenIterator > 0)
                {
                    screenIterator = (screenIterator - 1) % 40;
                }
                else if (screenIterator == 0)
                {
                    screenIterator = 39;
                }
            }
            ledcWrite(0, brightnessLevelsTable[0]); // Set brightness to max
        }
    }
}

void buttonPressHandler(volatile bool *buttonPressFlag)
{
    if (*buttonPressFlag)
    {
        *buttonPressFlag = false;
        brightnessLevel = (brightnessLevel + 1) % 6;
        ledcWrite(0, brightnessLevelsTable[brightnessLevel]);
        saveSetting(brightnessLevel, "brightness");

        showBrightnessChangeFlag = true;
        brightnessDebugPrintMillis = millis();
    }
}