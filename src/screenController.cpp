#include "screenController.h"

volatile int screenIterator = 0;
const int iteratorIncreaser = 0;
unsigned long brightnessDebugPrintMillis = 0;
bool showBrightnessChangeFlag = false;

volatile int brightnessLevel = 0; // index in table brightnessLevelsTable
int lastBrightnessLevel = 0;
const int brightnessLevelsTable[] = {255, 204, 153, 102, 51, 0}; // 100%, 80%, 60%, 40%, 20%, 0%

unsigned long real5sStartMillis = 0;
int last5sBlock = -1;

hw_timer_t *timerScreenIter = NULL;
hw_timer_t *timerLocalSensor = NULL;
uint16_t touchX, touchY;

struct tm timeinfo;

const char *daysOfWeek[] = {
    "Ndz",
    "Pon",
    "Wt ",
    "Sr ",
    "Czw",
    "Pt ",
    "Sob"};

void checkAndUpdateIcon(int x, int y)
{
    static String previousIcon = "";
    String currentIcon = weatherData[screenIterator + iteratorIncreaser].icon;
    if (currentIcon != previousIcon)
    {
        // tft.fillRect(10, heightOffset + 130, 100, 100, TFT_BLACK);
        tft.fillRect(x, y + 20, 100, 80, TFT_BLACK); // prev: x, y, 100, 100
        displayPNG(("/" + currentIcon + ".png").c_str(), x, y);
        previousIcon = currentIcon;
    }
}

void screenBrightnessChange()
{
    if (showBrightnessChangeFlag)
    {
        int percent = (brightnessLevelsTable[brightnessLevel] * 100) / 255;
        String percentStr = String(percent);
        static int oldPercent = -1;

        tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        tft.setTextSize(2);

        int16_t x = tft.width() - 48;
        int16_t y = tft.height() - 16;
        int16_t x_percent = tft.width() - tft.textWidth("%");
        int16_t x_number = x_percent - tft.textWidth(percentStr.c_str());

        if (percent != oldPercent)
        {
            tft.fillRect(x, y, 60, 16, TFT_BLACK);
            oldPercent = percent;
        }

        tft.setCursor(x_number, y);
        tft.print(percentStr);

        tft.setCursor(x_percent, y);
        tft.print("%");

        if (millis() - brightnessDebugPrintMillis > 5000)
        {
            tft.fillRect(x, y, 60, 16, TFT_BLACK);
            showBrightnessChangeFlag = false;
        }
    }
}

unsigned long realMillisInCurrent5sBlock()
{
    int current5sBlock = timeinfo.tm_sec / 5;
    if (current5sBlock != last5sBlock)
    {
        last5sBlock = current5sBlock;
        real5sStartMillis = millis();
    }
    return millis() - real5sStartMillis;
}

void drawSecondProgressBar()
{
    static int prev5sBlock = -1;
    unsigned long msInBlock = realMillisInCurrent5sBlock();
    if (msInBlock > 5000)
        msInBlock = 5000;

    int barWidth;
    const int barMax = 240;

    // Phase 1: 0-2.5s, filling in DARKGREY
    if (msInBlock < 2500)
    {
        barWidth = map(msInBlock, 0, 2500, 0, barMax);
        if (msInBlock == 0 || prev5sBlock != timeinfo.tm_sec / 5)
        {
            tft.fillRect(0, 0, barMax, 4, TFT_BLACK);
            prev5sBlock = timeinfo.tm_sec / 5;
        }
        tft.fillRect(0, 0, barWidth, 4, TFT_DARKGREY);
        if (msInBlock >= 2499)
        {
            tft.fillRect(0, 0, barMax, 4, TFT_DARKGREY);
        }
    }
    // Phase 2: 2.5-5s, filling in BLACK on top of darkgrey
    else
    {
        // First, ensure the background is filled with darkgrey (only once when entering this phase)
        static bool filledDarkGrey = false;
        if (!filledDarkGrey)
        {
            tft.fillRect(0, 0, barMax, 4, TFT_DARKGREY);
            filledDarkGrey = true;
        }
        barWidth = map(msInBlock, 2500, 5000, 0, barMax);
        tft.fillRect(0, 0, barWidth, 4, TFT_BLACK);
        if (msInBlock >= 4999)
        {
            tft.fillRect(0, 0, barMax, 4, TFT_BLACK);
            filledDarkGrey = false; // reset for next iteration
        }
        // Reset flag when entering a new block
        if (msInBlock < 2600)
            filledDarkGrey = false;
    }
}

void screenSaver()
{
    if ((timeinfo.tm_hour == 0 && timeinfo.tm_min == 0 && (timeinfo.tm_sec >= 0 && timeinfo.tm_sec < 10)) ||
        (timeinfo.tm_hour == 2 && timeinfo.tm_min == 0 && (timeinfo.tm_sec >= 0 && timeinfo.tm_sec < 10)) ||
        (timeinfo.tm_hour == 3 && timeinfo.tm_min == 0 && (timeinfo.tm_sec >= 0 && timeinfo.tm_sec < 10)) ||
        (timeinfo.tm_hour == 4 && timeinfo.tm_min == 0 && (timeinfo.tm_sec >= 0 && timeinfo.tm_sec < 10)))
    {
        if (brightnessLevel != 5)
        {
            // Serial.println("brightnessLevel > 0, setting brightness to 0 and sleeping screen");

            saveSetting(brightnessLevel, "brightnessLast"); // Store last brightness level for future unsleep
            // Serial.printf("saveSetting: %d as brightnessLast\n", brightnessLevel);

            brightnessLevel = 5;
            saveSetting(brightnessLevel, "brightness");
            // Serial.printf("saveSetting: %d as brightness\n", brightnessLevel);

            ledcWrite(0, brightnessLevelsTable[brightnessLevel]);
            // Serial.printf("ledcWrite: %d as brightness\n", brightnessLevelsTable[brightnessLevel]);

            screenSleepingFlag = true;
            // Serial.println("Screen sleeping now");
        }
    }
}

void screenController()
{
    drawSecondProgressBar();

    // Font Height is 8 pixels for size 1, 16 for size 2 etc, thats why HEIGHT is multiplied by 8 + 2 for spacing
    char buffer[50];
    int heightOffset = 20; // 20

    tft.setTextColor(TFT_WHITE, TFT_BLACK, true);

    // 1st line - time
    tft.setTextSize(5);
    sprintf(buffer, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    drawCenteredText(String(buffer), heightOffset + 0);

    // 2nd line - date
    tft.setTextSize(2);
    sprintf(buffer, "%02d-%02d %s", timeinfo.tm_mday, timeinfo.tm_mon + 1, daysOfWeek[timeinfo.tm_wday]);
    drawCenteredText(String(buffer), heightOffset + (8 * 5 + 2));

    checkAndUpdateTempAndHumidity(buffer, heightOffset);

    // 4th line - sunrise and sunset
    tft.setTextSize(2);
    sprintf(buffer, "%s  %s", weatherData[0].time.substring(0, 5).c_str(), weatherData[0].time.substring(6, 11).c_str());
    drawCenteredText(String(buffer), heightOffset + (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2));

    tft.drawLine(20, heightOffset + (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2), 220, heightOffset + (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2), TFT_WHITE);
    //-----------------------------------------------------------------------------------------------------------------------//  LINE on 104-105
    if (screenIterator > 0)
    {
        tft.setTextSize(2);
        sprintf(buffer, "%s", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str());
        drawCenteredText(String(buffer), heightOffset + 108);
    }
    else
    {
        tft.setTextSize(2);
        sprintf(buffer, "%s", "           ");
        drawCenteredText(String(buffer), heightOffset + 108);
    }

    tft.setTextSize(1);
    tft.setCursor(130, heightOffset + 108 + (8 * 2 + 4));
    tft.printf("%s", "temp. rzeczywista");

    tft.setTextSize(3);
    sprintf(buffer, "%5.1f'C", weatherData[screenIterator + iteratorIncreaser].temp); // max 5 chars at width
    tft.setTextDatum(TR_DATUM);                                                       // TR_DATUM - Top right
    tft.drawString(String(buffer), 240, heightOffset + 108 + (8 * 2 + 4) + (8 * 1 + 2));

    tft.setTextSize(1);
    tft.setCursor(130, heightOffset + 108 + (8 * 2 + 4) + (8 * 1 + 2) + (8 * 3 + 10));
    tft.printf("%s", "temp. odczuwalna");

    tft.setTextSize(3);
    sprintf(buffer, "%5.1f'C", weatherData[screenIterator + iteratorIncreaser].feelsLike); // max 5 chars at width
    tft.setTextDatum(TR_DATUM);
    tft.drawString(String(buffer), 240, heightOffset + 108 + (8 * 2 + 4) + (8 * 1 + 2) + (8 * 3 + 10) + (8 * 1 + 2));

    checkAndUpdateIcon(10, heightOffset + 110);

    tft.setTextSize(2);
    tft.setCursor(15, heightOffset + 235);
    tft.printf("Wiatr: %3.0f km/h\n", weatherData[screenIterator + iteratorIncreaser].windSpeed * 3.6f);
    tft.setCursor(15, heightOffset + 253);
    tft.printf("Deszcz: %3.1f mm\n", weatherData[screenIterator + iteratorIncreaser].rain);
    tft.setCursor(15, heightOffset + 271);
    tft.printf("Snieg: %4.1f mm\n", weatherData[screenIterator + iteratorIncreaser].snow);

    // Additional line- time from uC start
    tft.setTextSize(1);
    unsigned long uptimeMillis = millis();
    unsigned long seconds = uptimeMillis / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    seconds %= 60;
    minutes %= 60;
    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    drawCenteredText(String(buffer), 320 - 8 - 1);
}