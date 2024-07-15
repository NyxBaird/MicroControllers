#include <Arduino.h>

#define HES 6
#define HES2 7

unsigned long lastRPMCalculation;
int lastHESState;
int lastHES2State;
unsigned long motorRotations;
unsigned long motorRotations2;
unsigned long currentTime;

void setup() {
    Serial.begin(115200);

    pinMode(HES, INPUT);
    pinMode(HES2, INPUT);

    lastRPMCalculation = millis();
    lastHESState = digitalRead(HES);
    lastHES2State = digitalRead(HES2);
    motorRotations = 0;
    motorRotations2 = 0;
}

void loop() {
    int currentHESState = digitalRead(HES);
    if (lastHESState != currentHESState) {
        motorRotations++;
        lastHESState = currentHESState;
    }

    int currentHES2State = digitalRead(HES2);
    if (lastHES2State != currentHES2State) {
        motorRotations2++;
        lastHES2State = currentHES2State;
    }

    currentTime = millis();
    if (currentTime - lastRPMCalculation > 5000) { // Calculate RPM every 5 seconds (5000 ms)
        unsigned long rpm1 = (motorRotations * 60) / 2;
        unsigned long rpm2 = (motorRotations2 * 60) / 2;

        Serial.print("RPM of HES1: ");
        Serial.print(rpm1);
        Serial.print(" | RPM of HES2: ");
        Serial.println(rpm2);

        motorRotations = 0;
        motorRotations2 = 0;
        lastRPMCalculation = currentTime;
    }
}
