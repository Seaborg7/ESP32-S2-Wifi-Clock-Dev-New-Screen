// ////////5.04.2025, działa dla testowych ikon bmp

// #include <arduino.h>
// #include <SPIFFS.h>
// #include <TFT_eSPI.h>

// TFT_eSPI tft = TFT_eSPI(); // TFT konstruktor

// // Pomocnicze funkcje do odczytu danych BMP
// uint16_t read16(File &f)
// {
//     uint16_t result;
//     ((uint8_t *)&result)[0] = f.read(); // LSB
//     ((uint8_t *)&result)[1] = f.read(); // MSB
//     return result;
// }

// uint32_t read32(File &f)
// {
//     uint32_t result;
//     ((uint8_t *)&result)[0] = f.read();
//     ((uint8_t *)&result)[1] = f.read();
//     ((uint8_t *)&result)[2] = f.read();
//     ((uint8_t *)&result)[3] = f.read();
//     return result;
// }

// // Funkcja do wczytywania i wyświetlania BMP
// void drawBMP(const char *filename, int16_t x, int16_t y)
// {
//     File bmpFile = SPIFFS.open(filename, "r");
//     if (!bmpFile)
//     {
//         Serial.println("Failed to open BMP file");
//         return;
//     }

//     // Sprawdzenie nagłówka BMP
//     if (read16(bmpFile) != 0x4D42)
//     {
//         Serial.println("Not a BMP file");
//         bmpFile.close();
//         return;
//     }

//     // Pomiń niepotrzebne dane z nagłówka
//     read32(bmpFile);                        // file size
//     read32(bmpFile);                        // creator bytes
//     uint32_t imageOffset = read32(bmpFile); // offset do danych obrazu
//     read32(bmpFile);                        // DIB header size
//     int32_t bmpWidth = read32(bmpFile);
//     int32_t bmpHeight = read32(bmpFile);
//     if (read16(bmpFile) != 1)
//     {
//         bmpFile.close();
//         return;
//     }
//     uint16_t bmpDepth = read16(bmpFile);
//     if (bmpDepth != 24)
//     {
//         Serial.println("Only 24-bit BMP supported.");
//         bmpFile.close();
//         return;
//     }
//     if (read32(bmpFile) != 0)
//     {
//         Serial.println("Compressed BMP not supported.");
//         bmpFile.close();
//         return;
//     }

//     // Ustaw plik na początek danych pikseli
//     bmpFile.seek(imageOffset);

//     // BMP jest zapisywany od dołu do góry
//     bool flip = true;
//     if (bmpHeight < 0)
//     {
//         bmpHeight = -bmpHeight;
//         flip = false;
//     }

//     uint32_t rowSize = (bmpWidth * 3 + 3) & ~3;

//     uint8_t sdbuffer[3 * 20]; // bufor na 20 pikseli
//     for (int row = 0; row < bmpHeight; row++)
//     {
//         int pos = flip ? bmpHeight - 1 - row : row;
//         bmpFile.seek(imageOffset + pos * rowSize);
//         for (int col = 0; col < bmpWidth; col++)
//         {
//             bmpFile.read(sdbuffer, 3);
//             uint8_t b = sdbuffer[0];
//             uint8_t g = sdbuffer[1];
//             uint8_t r = sdbuffer[2];
//             uint16_t color = tft.color565(r, g, b);
//             tft.drawPixel(x + col, y + row, color);
//         }
//     }

//     bmpFile.close();
// }

// void setup()
// {
//     Serial.begin(115200);

//     // Inicjalizacja TFT
//     tft.init();
//     tft.setRotation(2); // Obrót ekranu, jeśli potrzeba

//     // Inicjalizacja SPIFFS
//     if (!SPIFFS.begin())
//     {
//         Serial.println("SPIFFS Mount Failed");
//         return;
//     }

//     // Wczytaj i wyświetl obraz
//     drawBMP("/1.bmp", 10, 10);
//     drawBMP("/10d_t@2x.bmp", 120, 120);

//     size_t totalBytes = SPIFFS.totalBytes();
//     size_t usedBytes = SPIFFS.usedBytes();
//     size_t freeBytes = totalBytes - usedBytes;

//     Serial.println("=== SPIFFS Info ===");
//     Serial.printf("Total: %d bytes\n", totalBytes);
//     Serial.printf("Used:  %d bytes\n", usedBytes);
//     Serial.printf("Free:  %d bytes\n", freeBytes);
// }

// void loop()
// {
//     // Nic w pętli
// }