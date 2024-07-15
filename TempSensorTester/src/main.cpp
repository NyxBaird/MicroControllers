#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h> // Add the library for the I2C LCD

// Data wire is connected to pins 2 and 3
#define ONE_WIRE_BUS_1 A0
#define ONE_WIRE_BUS_2 A2

#define COOLER_POWER A1

OneWire oneWire1(ONE_WIRE_BUS_1);
OneWire oneWire2(ONE_WIRE_BUS_2);
DallasTemperature sensors1(&oneWire1);
DallasTemperature sensors2(&oneWire2);

// Define the I2C LCD connection
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address and dimensions based on your LCD module

void setup() {
    Serial.begin(9600);
    Serial.println("LCD Test!");

    sensors1.begin();
    sensors2.begin();

    pinMode(A1, OUTPUT);

    lcd.begin();
}

float lastTemp1 = 0;
float lastTemp2 = 0;

int coolantCutoff = 95; //The coolant temp at which our whole system shuts off
int tempShutoff = 45; //The air temp at which our whole system shuts off

void loop(void) {

    sensors1.requestTemperatures();
    sensors2.requestTemperatures();

    float temp1 = sensors1.getTempFByIndex(0);
    float temp2 = sensors2.getTempFByIndex(0);

    Serial.print("Temperature Sensor 1: ");
    Serial.print(temp1);
    Serial.println("F");

    Serial.print("Temperature Sensor 2: ");
    Serial.print(temp2);
    Serial.println("F");

    lcd.clear();

    // Display temperature Sensor 1 if it is a valid reading
    if (temp1 > -127 && temp1 < 200)
        lastTemp1 = temp1;


    // Display temperature Sensor 2 if it is a valid reading
    if (temp2 > -127 && temp2 < 200)
        lastTemp2 = temp2;

    lcd.setCursor(0, 0);
    lcd.print("Coolant :");
    lcd.print(String(lastTemp1, 2));
    lcd.print("F");

    lcd.setCursor(0, 1);
    lcd.print("External:");
    lcd.print(String(lastTemp2, 2));
    lcd.print("F");

    if (lastTemp1 > coolantCutoff || lastTemp2 < tempShutoff)
        digitalWrite(A1, HIGH);
    else
        digitalWrite(A1, LOW);

    delay(1000);
}
