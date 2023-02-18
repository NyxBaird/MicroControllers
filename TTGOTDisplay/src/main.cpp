#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
    tft.init();
    tft.setRotation(3);
    tft.setSwapBytes(true);

    tft.fillScreen(TFT_BLACK);
}

void loop() {

}