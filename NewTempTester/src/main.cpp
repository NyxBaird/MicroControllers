#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN A0
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  float temp = dht.readTemperature(true); // true for Fahrenheit

  if (isnan(temp)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println("F");
  }

  delay(2000);
}
