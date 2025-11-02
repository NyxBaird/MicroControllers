#include <Arduino.h>
#include <Wire.h>

#define TCA9548A_ADDRESS 0x70 // Default address for TCA9548A

void setup() {
    Wire.begin();  // Initialize the default I2C bus
    Wire1.begin(); // Initialize the second I2C bus

    Serial.begin(9600);
    while (!Serial); // Wait for Serial to initialize
    Serial.println("\nI2C Scanner for TCA9548A");
}

void tcaSelect(uint8_t channel) {
    if (channel > 7) return; // Ensure channel is valid
    Wire1.beginTransmission(TCA9548A_ADDRESS);
    Wire1.write(1 << channel); // Select the channel
    Wire1.endTransmission();
}

void scanBus(TwoWire &bus, const char* busName) {
    byte error, address;
    int nDevices = 0;

    Serial.print("Scanning ");
    Serial.print(busName);
    Serial.println("...");

    for (address = 1; address < 127; address++) {
        bus.beginTransmission(address);
        error = bus.endTransmission();

        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.print(address, HEX);
            Serial.println(" !");

            nDevices++;
        } else if (error == 4) {
            Serial.print("Unknown error at address 0x");
            if (address < 16)
                Serial.print("0");
            Serial.println(address, HEX);
        }
    }

    if (nDevices == 0)
        Serial.println("No I2C devices found\n");
    else
        Serial.println("done\n");
}

void loop() {
    for (uint8_t channel = 0; channel < 8; channel++) {
        Serial.print("Selecting channel ");
        Serial.println(channel);
        tcaSelect(channel); // Select the channel on the multiplexer
        scanBus(Wire1, "Wire1"); // Scan the selected channel
        delay(1000); // Short delay between channel scans
    }

    delay(5000); // wait 5 seconds before scanning again
}
