#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define MAX6675_CS   4
#define MAX6675_SO   5
#define MAX6675_SCK  3

// Initialize the LCD display at address 0x27 with 4 rows and 20 columns
LiquidCrystal_I2C lcd(0x27, 20, 4);

double readThermocouple() {

    uint16_t v;
    pinMode(MAX6675_CS, OUTPUT);
    pinMode(MAX6675_SO, INPUT);
    pinMode(MAX6675_SCK, OUTPUT);

    digitalWrite(MAX6675_CS, LOW);
    delay(1);

    v = shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);
    v <<= 8;
    v |= shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);

    digitalWrite(MAX6675_CS, HIGH);
    if (v & 0x4)
    {
        return NAN;
    }

    v >>= 3;

    // Convert Celsius to Fahrenheit
    return v*0.25 * 9.0/5.0 + 32.0;
}

void setup() {
    Serial.begin(115200);
    lcd.begin();
    lcd.backlight(); // turn on backlight
}

void loop() {
    double temperature_read = readThermocouple();
    Serial.println("Temp: " + String(temperature_read) + "F");

    // Print temperature to LCD
    lcd.setCursor(0, 0); // set cursor to first column, first row
    lcd.print("Temp: ");
    lcd.print(temperature_read);
    lcd.print("F");

    delay(1000);
}
