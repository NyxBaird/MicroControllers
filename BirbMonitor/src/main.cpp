#include <Arduino.h>

#include "Adafruit_Sensor.h"
#include "DHT.h"
DHT dht(32, DHT11);

#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

int soundDetectorPin = 36;

int runtime = 0;
int lastTempCheck = 0;

void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(soundDetectorPin, INPUT);

    SerialBT.begin("Birbo Monitor");
    Serial.println("Started server...");
}

void loop() {

    if (Serial.available()) {
        SerialBT.write(Serial.read());
    }
    if (SerialBT.available()) {
        Serial.write(SerialBT.read());
    }

    if (!digitalRead(soundDetectorPin)) {
        Serial.println("Noise level greater than limit!");
        //send alert

        //Give birbo time to settle before letting us send the alert again
        delay(1000);
    }

    if (lastTempCheck < runtime - 50000) {
        // Reading temperature or humidity takes about 250 milliseconds!
        // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
        float h = dht.readHumidity();
        // Read temperature as Celsius (the default)
        float t = dht.readTemperature();
        // Read temperature as Fahrenheit (isFahrenheit = true)
        float f = dht.readTemperature(true);

        // Check if any reads failed and exit early (to try again).
        if (isnan(h) || isnan(t) || isnan(f)) {
            return;
        }

        // Compute heat index in Fahrenheit (the default)
        float hif = dht.computeHeatIndex(f, h);
        // Compute heat index in Celsius (isFahreheit = false)
        float hic = dht.computeHeatIndex(t, h, false);

        Serial.print(F("Humidity: "));
        Serial.print(h);
        Serial.print(F("%  Temperature: "));
        Serial.print(t);
        Serial.print(F("°C "));
        Serial.print(f);
        Serial.print(F("°F  Heat index: "));
        Serial.print(hic);
        Serial.print(F("°C "));
        Serial.print(hif);
        Serial.println(F("°F"));

        lastTempCheck = runtime;
    }

    runtime++;
}