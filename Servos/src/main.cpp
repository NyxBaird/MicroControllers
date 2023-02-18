#include <Arduino.h>
#include <ESP32Servo.h>

Servo servo;
int pos = 0;    // variable to store the servo position

void setup() {
    Serial.begin(115200);
    servo.attach(2);
}

void loop() {
    servo.write(0);
    delay(10000);
    servo.write(180);
    delay(5000);

}