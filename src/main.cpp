#include "main.h"

// Comment out to stop drawing red spots
// #define RED_SPOT // RED_SPOT is causing to read "tft.getTouch" which leads to falling edge on pin T_IRQ

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // Adjust to your time zone
const int daylightOffset_sec = 3600;

volatile unsigned long timerScreenIterDelay = 5000000;  // 5 seconds
volatile unsigned long timerLocalSensorDelay = 2000000; // 2 seconds
volatile unsigned long taskWeatherDataDelay = 600000;   // 60 seconds

void drawDotsOnTouch()
{
#ifdef RED_SPOT
    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
        tft.fillCircle(x, y, 2, TFT_RED);
    }
#endif
}

//------------------------------------------------------------------------------------------
void setup(void)
{
    // Serial.begin(9600);
    WifiInit();
    dht.begin();
    TftInit();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Button configuration
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleBrightnessButton, FALLING); // Set up interrupt for button press on falling edge

    pinMode(LED_PIN, OUTPUT);
    pinMode(TOUCH_IRQ, INPUT);
    attachInterrupt(digitalPinToInterrupt(TOUCH_IRQ), handleScreenPress, FALLING);

    fetchCurrentWeather();
    fetchWeatherForecast();

    timerScreenIter = timerBegin(0, 80, true);                       // Timer 0, prescaler 80, counter in up-count mdoe
    timerAttachInterrupt(timerScreenIter, &onTimerScreenIter, true); // Setup interrupt
    timerAlarmWrite(timerScreenIter, timerScreenIterDelay, true);    // 5 seconds
    timerAlarmEnable(timerScreenIter);                               // Alarm enable

    timerLocalSensor = timerBegin(1, 80, true);                        // Timer 1, prescaler 80, counter in up-count mdoe
    timerAttachInterrupt(timerLocalSensor, &onTimerLocalSensor, true); // Setup interrupt
    timerAlarmWrite(timerLocalSensor, timerLocalSensorDelay, true);    // 2 seconds
    timerAlarmEnable(timerLocalSensor);                                // Alarm enable

    printAllTnternalFileList();
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void loop()
{
    drawDotsOnTouch(); // #ifdef RED_SPOT

    callAfterDelay(taskWeatherData);

    localTimeHandler(&timeinfo);
    localSensorMeasurementsHandler(&readSensorFlag);

    screenPressHandler(&screenPressFlag);
    buttonPressHandler(&buttonPressFlag);

    screenController();
    screenBrightnessChange();
    screenSaver(&screenSleepingFlag, &brightnessLevel);
}
//------------------------------------------------------------------------------------------