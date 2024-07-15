#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define RGB_PIN D1
#define NUM_PIXELS 41

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
    Serial.begin(115200);
    Serial.println("heya");
    pixels.begin();

    for(int i=0; i<NUM_PIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 255));
        pixels.show();
        delay(50);
    }
}

void loop() {

}