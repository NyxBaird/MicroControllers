#include <Arduino.h>
void setup() {
    Serial1.begin(115200); // Initialize Serial1 at 115200 baud rate
    pinMode(0, INPUT); // Set RX_PIN as INPUT
}

void loop() {
    if (Serial1.available()) { // Check if data is available to read
        char c = Serial1.read(); // Read a character from Serial1
        Serial1.write(c); // Echo the character back to Serial1
    }
}