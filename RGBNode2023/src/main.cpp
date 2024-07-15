#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

//Potentiometer vars
const int potPin = 34;
const float maxPotValue = 4095.0;
int potValue = 0;

//Photoresistor vars
#define PR_PIN 36
int lightInit;  // initial value
int lightVal;   // current value

//Button vars
#define RGB_PIN 13
#define BTN_PIN 39
#define SYS_BTN 0

//Pixel vars
const int pixelCount = 200;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(pixelCount, RGB_PIN, NEO_GRB + NEO_KHZ800);

//Lighting vars
int pixelColors[pixelCount][4]; //pixelColors[pixel] = [red, green, blue, brightness%]
int globalBrightness = 100; //0-255
int nodeState = 0; //0 = off; 1 = user color; 2 = white

void throwError(String error) {
    //Add error led code here
    Serial.println("ERROR: " + error);
}

//Expects:
//led - The id of the LED to change
//red - 0-255 color value
//green - 0-255 color value
//blue - 0-255 color value
void setColor(int led, int red, int green, int blue)
{
    if (pixelColors[led][0] == red && pixelColors[led][1] == green && pixelColors[led][2] == blue)
        return;

    pixelColors[led][0] = red;
    pixelColors[led][1] = green;
    pixelColors[led][2] = blue;

    pixels.setPixelColor(led, pixels.Color(pixelColors[led][0], pixelColors[led][1], pixelColors[led][2]));
    pixels.show();
}

//Fill the whole strip with a color
//Expects:
//led - The id of the LED to change
//red - 0-255 color value
//green - 0-255 color value
//blue - 0-255 color value
//startPixel - The pixel to start on
//count - how many pixels to color (defaults to all available)
void setStripColor(int red, int green, int blue, int startPixel = 0, int count = -1);
void setStripColor(int red, int green, int blue, int startPixel, int count)
{
    int changed = 0;
    for (int i = startPixel; i < pixelCount; i++) {
        if (count > -1 && changed >= count)
            return;
        else
            setColor(i, red, green, blue);

        changed++;
    }
}

void restorePixelColors()
{
    for (int i = 0; i < pixelCount; i++)
        pixels.setPixelColor(i, pixels.Color(pixelColors[i][0], pixelColors[i][1], pixelColors[i][2]));

    pixels.show();
} //Undoes setStripWhite
void setStripWhite()
{
    Serial.println("Setting strip white:" + String(globalBrightness));

    pixels.fill(pixels.Color(255, 255, 255), 0, pixelCount);
    pixels.setBrightness(globalBrightness);
    pixels.show();
} //Sets a special "white light" state
void turnStripOff()
{
//    pixels.fill(pixels.Color(1, 1, 1), 0, pixelCount);
    pixels.setBrightness(1);
    pixels.show();
}


void setup() {
    Serial.begin(9600);

    //Get our initial light level
    lightInit = analogRead(PR_PIN);
    Serial.println("Light Init: " + String(lightInit));

    //Set our button mode
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(SYS_BTN, INPUT_PULLUP);

    pixels.begin();
    setColor(0, 0, 0, 3);

    //Set state to user and set an initial user state of blue
    nodeState = 1;
    setStripColor(0, 0, 255);
}

void loop() {

    if (!digitalRead(SYS_BTN)) {
        Serial.println("Light: " + String(lightVal));

        if (nodeState > 0)
            nodeState = 0;
        else
            nodeState = 1;

        if (nodeState == 1) {
            setStripWhite();
        } else
            turnStripOff();

        yield();
        delay(500);
    }

    //Fetch our current brightness from the potentiometer
    potValue = analogRead(potPin);
    int newBrightness = (float)potValue / maxPotValue * 100;
    newBrightness = (255 / 100) * newBrightness;

    if (newBrightness != globalBrightness) {
        Serial.println("New Brightness: " + String(newBrightness) + " | " + String(globalBrightness) + " | " + String(newBrightness != globalBrightness));
        if (newBrightness < 1)
            newBrightness = 1;

        globalBrightness = newBrightness;
        pixels.setBrightness(globalBrightness);
        pixels.show();
    }
}