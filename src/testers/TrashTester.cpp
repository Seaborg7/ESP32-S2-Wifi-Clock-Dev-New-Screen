// //// 5.04.2025 DZIAŁA // tester dla plików PNG

// //// =====================================================================================================================================================
// #include <Arduino.h>
// #include <SPIFFS.h>
// #include <TFT_eSPI.h>
// #include <PNGdec.h>

// TFT_eSPI tft = TFT_eSPI();
// PNG png;

// #define MAX_IMAGE_WIDTH 320

// File pngFile;

// // Zmienne do przechowywania pozycji obrazu
// int imageX = 0; // Pozycja X obrazu
// int imageY = 0; // Pozycja Y obrazu

// // Funkcje callback dla PNGdec
// void *pngOpen(const char *filename, int32_t *size)
// {
//     Serial.printf("Opening %s\n", filename);
//     pngFile = SPIFFS.open(filename, "rb");
//     *size = pngFile.size();
//     return &pngFile;
// }

// void pngClose(void *handle)
// {
//     if (pngFile)
//         pngFile.close();
// }

// int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
// {
//     return pngFile.read(buffer, length);
// }

// int32_t pngSeek(PNGFILE *page, int32_t position)
// {
//     return pngFile.seek(position);
// }

// void pngDraw(PNGDRAW *pDraw)
// {
//     uint16_t lineBuffer[MAX_IMAGE_WIDTH];
//     png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0x00000000); // Czarny jako przezroczysty

//     for (int i = 0; i < pDraw->iWidth; i++)
//     {
//         uint16_t color = lineBuffer[i];
//         if (color != 0x0000) // Pomijamy czarne piksele (przezroczyste)
//         {
//             tft.drawPixel(imageX + i, imageY + pDraw->y, color);
//         }
//     }
// }

// // Funkcja do wyświetlania obrazu w określonej pozycji
// void displayPNG(const char *filename, int x, int y)
// {
//     imageX = x;
//     imageY = y;

//     int rc = png.open(filename, pngOpen, pngClose, pngRead, pngSeek, pngDraw);
//     if (rc == PNG_SUCCESS)
//     {
//         Serial.printf("Displaying image at (%d, %d)\n", x, y);
//         Serial.printf("Image size: %d x %d\n", png.getWidth(), png.getHeight());

//         uint32_t start = millis();
//         rc = png.decode(NULL, 0);
//         Serial.printf("Decode time: %d ms\n", millis() - start);

//         png.close();
//     }
//     else
//     {
//         Serial.printf("PNG error: %d\n", rc);
//     }
// }

// void setup()
// {
//     Serial.begin(115200);
//     delay(1000);

//     if (!SPIFFS.begin(true))
//     {
//         Serial.println("SPIFFS init failed!");
//         while (1)
//             ;
//     }

//     tft.begin();
//     tft.setRotation(1);
//     tft.fillScreen(TFT_BLUE);

//     // Przykłady użycia:
//     displayPNG("/1.png", 10, 20); // Lewy górny róg (10,20)
//     delay(2000);

//     tft.fillScreen(TFT_RED);
//     displayPNG("/13d_t@2x.png", 50, 100); // Środek ekranu (50,100)
// }

// void loop()
// {
//     // Możesz tutaj zmieniać pozycję obrazu dynamicznie
//     // np. w odpowiedzi na przyciski czy dotyk
//     delay(1000);
// }