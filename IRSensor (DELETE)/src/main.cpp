#include <Arduino.h>
#include <IRremote.hpp>

void setup() {
    Serial.begin(115200);

    IrReceiver.begin(37);
}

void loop() {
// write your code here
//handle ir commands
    if (IrReceiver.decode()) {
        if (IrReceiver.decodedIRData.decodedRawData > 0) {
            char code[9];
            itoa(IrReceiver.decodedIRData.decodedRawData, code, 16);
            Serial.println(code);
        }

        IrReceiver.resume(); // Enable receiving of the next value
    }
}