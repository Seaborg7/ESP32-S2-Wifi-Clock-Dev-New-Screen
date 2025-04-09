// #include <Arduino.h>
// #include <DHT.h>

// #define DHT_PIN 39
// #define DHT_TYPE DHT11

// DHT dht(DHT_PIN, DHT_TYPE);

// void setup()
// {
//     Serial.begin(115200);
//     Serial.println("Inicjalizacja DHT11...");

//     dht.begin();
// }

// void loop()
// {
//     float temperature = dht.readTemperature();
//     float humidity = dht.readHumidity();

//     if (isnan(temperature) || isnan(humidity))
//     {
//         Serial.println("Błąd odczytu z czujnika DHT11!");
//     }
//     else
//     {
//         Serial.printf("Temperatura: %.1f °C  Wilgotność: %.1f %%\n", temperature, humidity);
//     }

//     delay(500);
// }