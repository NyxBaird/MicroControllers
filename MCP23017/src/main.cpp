#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <IRremote.hpp>
#include <TFT_eSPI.h>

Adafruit_MCP23X17 mcp = Adafruit_MCP23X17();


void setup() {
    Serial.begin(115200);
    if (!mcp.begin_I2C()) {
        //if (!mcp.begin_SPI(CS_PIN)) {
        Serial.println("Error.");
        while (1);
    }
    mcp.pinMode(8, INPUT_PULLUP);
}

void loop() {
    Serial.println("btn: " + String(mcp.digitalRead(8)));

}