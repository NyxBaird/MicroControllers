#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

const int Sensor1 = 27;

void drawScreen(String message = "No message");
void drawScreen(String message) {
    Serial.println("Drawing screen...");

    //Draw our background
    tft.fillScreen(TFT_BLACK);

    //Draw IP and version
    tft.setTextColor(TFT_WHITE);
    tft.drawString(message, 3, 3, 4);
}

void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(3);
    tft.setSwapBytes(true);

    drawScreen("Loading...");

    pinMode(Sensor1, INPUT_PULLUP);
}

void loop() {
    Serial.println("Sensor: " + String(analogRead(Sensor1)));
    delay(500);
}