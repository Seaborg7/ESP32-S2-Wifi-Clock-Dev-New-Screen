#include "screenController.h"
#include "handlingFuntions.h"
#include "weatherHandler.h"

volatile int screenIterator = 0;
const int iteratorIncreaser = 0;

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
        // tft.fillRect(10, heightOffset + 130, 100, 100, TFT_BLACK); // Wyczyść obszar, rysując czarny kwadrat
        tft.fillRect(x, y + 20, 100, 80, TFT_BLACK); // prev: x, y, 100, 100
        displayPNG(("/" + currentIcon + ".png").c_str(), x, y);
        previousIcon = currentIcon;
    }
}

void ScreenController()
{
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

    // tft.setCursor(25, (8 * 5 + 2) + (8 * 2 + 2));
    // tft.setTextSize(3);
    // tft.printf("%.1f'C", temperature);
    // tft.setCursor(160, (8 * 5 + 2) + (8 * 2 + 2));
    // tft.printf("%.0f%% ", humidity);

    // 3rd line - temperature and humidity
    // tft.setTextSize(3);
    // sprintf(buffer, "%.1f'C  %.0f%%", temperature, humidity);
    // drawCenteredText(String(buffer), (8 * 5 + 2) + (8 * 2 + 2));
    checkAndUpdateTempAndHumidity(buffer, heightOffset);

    // tft.setCursor(60, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2));
    // tft.setTextSize(2);
    // tft.printf("%s  %s", weatherData[0].time.substr(0, 4).c_str(), weatherData[0].time.substr(5, 10).c_str());

    // 4th line - sunrise and sunset
    tft.setTextSize(2);
    sprintf(buffer, "%s  %s", weatherData[0].time.substring(0, 5).c_str(), weatherData[0].time.substring(6, 11).c_str());
    drawCenteredText(String(buffer), heightOffset + (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2));

    // tft.drawFastHLine(200, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2), 210, TFT_WHITE);
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

    // // Small iterable text on bottom half of the screen //
    // // tft.setCursor(0, (8 * 5 + 2) + (8 * 2 + 2) + (8 * 3 + 2) + (8 * 2 + 2) + 100);
    // // tft.setTextSize(2);
    // // tft.printf("%s %.1f %.1f\n", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str(), weatherData[screenIterator + iteratorIncreaser].temp, weatherData[screenIterator + iteratorIncreaser].feelsLike);
    // // tft.printf("%s\n", weatherData[screenIterator + iteratorIncreaser].description.c_str());
    // // tft.printf("%.0f km/h %.1f %.1f mm\n", weatherData[screenIterator + iteratorIncreaser].windSpeed * 3.6f, weatherData[screenIterator + iteratorIncreaser].rain, weatherData[screenIterator + iteratorIncreaser].snow);

    // // tft.printf("%s %s\n", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str(), weatherData[screenIterator + iteratorIncreaser].description.c_str());
    // // tft.printf("Temp: %.1f 'C Feels like: %.1f 'C\n", weatherData[screenIterator + iteratorIncreaser].temp, weatherData[screenIterator + iteratorIncreaser].feelsLike);
    // // tft.printf("Wind %.2f m/s Rain(3h) %.2f mm\n", weatherData[screenIterator + iteratorIncreaser].windSpeed, weatherData[screenIterator + iteratorIncreaser].rain);

    // Serial.printf("Time: %s   ScreenIter: %d\n", formatDateTime(weatherData[screenIterator + iteratorIncreaser].time).c_str(), screenIterator);
    // Serial.printf("Description: %s \n", weatherData[screenIterator + iteratorIncreaser].description.c_str());
    // Serial.printf("Temp: %.1f 'C Feels like: %.1f 'C\n", weatherData[screenIterator + iteratorIncreaser].temp, weatherData[screenIterator + iteratorIncreaser].feelsLike);
    // Serial.printf("Wind %.0f km/h Rain(3h) %.1f mm Snow(3h) %.1f mm\n", weatherData[screenIterator + iteratorIncreaser].windSpeed * 3.6f, weatherData[screenIterator + iteratorIncreaser].rain, weatherData[screenIterator + iteratorIncreaser].snow);
}