#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

// Define the pins for the display
#define TFT_MOSI 26
#define TFT_SCLK 27
#define TFT_CS   30
#define TFT_DC   31
#define TFT_RST  32
#define TFT_BL   33

// Create an instance of the display
Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_RST);

void setup() {
    // Initialize the display
    tft.begin();

    // Set the backlight pin
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); // Turn on the backlight

    // Set rotation if needed
    tft.setRotation(0);

    // Fill the screen with a color
    tft.fillScreen(GC9A01A_BLACK);

    // Set text color and size
    tft.setTextColor(GC9A01A_WHITE);
    tft.setTextSize(2);

    // Display text
    tft.setCursor(10, 10);
    tft.println("Hello, World!");
}

void loop() {
// write your code here
}