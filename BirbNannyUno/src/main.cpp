#include <Adafruit_Sensor.h>

#include <DHT.h>
DHT dht(0, DHT11, true);

#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define USE_Elegoo_SHIELD_PINOUT

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void drawScreen() {
    tft.setCursor(1, 1);
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.println("Font 1");
    tft.setTextSize(2);
    tft.println("Font 2");
    tft.setTextSize(3);
    tft.println("Font 3");
    tft.setTextSize(4);
    tft.println("Font 4");
    tft.setTextSize(5);
    tft.println("Font 5");
    tft.setTextSize(6);
    tft.println("Font 6");
    tft.setTextSize(7);
    tft.println("Font 7");
    tft.setTextSize(8);
    tft.println("Font 8");
}

void setup() {
    Serial.begin(9600);

    Serial.print("TFT size is ");
    Serial.println(String(tft.width()) + "x" + String(tft.height()));

    dht.begin();

    tft.reset();
    tft.begin(0x9341);
//    tft.setRotation(1);
    tft.setRotation(3);
}

void loop() {
    float h = dht.readHumidity();
    float f = dht.readTemperature(true);

    if (isnan(h) || isnan(f)) {
        Serial.println(F("Failed to read from DHT sensor!"));
        return;
    }

    Serial.print(F(" Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(F("°F "));
    Serial.print(f);

    drawScreen();
    delay(2000);
}


