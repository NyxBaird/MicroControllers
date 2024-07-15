#include <Arduino.h>
#include <HCSR04.h>
#include <RH_ASK.h>

#define TX_PIN 1
#define RX_PIN 0
RH_ASK driver(            2000, RX_PIN, TX_PIN);

UltraSonicDistanceSensor sensor(18, 17);

//Ultrasonic;
//Echo; 9

struct Wheel {
    int fwdPin;
    int bckPin;

    Wheel(int fwd, int bck) : fwdPin(fwd), bckPin(bck) {}

    void init() const {
        pinMode(fwdPin, OUTPUT);
        pinMode(bckPin, OUTPUT);
    }

    void forward(int speed) const {
        digitalWrite(fwdPin, HIGH);
        digitalWrite(bckPin, LOW);
    }
    void backward(int speed) const {
        digitalWrite(fwdPin, LOW);
        digitalWrite(bckPin, HIGH);
    }
};
struct Driver { //Each driver runs two wheels
    Wheel front;
    Wheel back;

    int coastPin;
    int brakePin;

    Driver(const Wheel& f, const Wheel& b, int coast, int brake)
            : front(f), back(b), coastPin(coast), brakePin(brake) {}

    void init() const {
        front.init();
        back.init();
        pinMode(coastPin, OUTPUT);
        pinMode(brakePin, OUTPUT);
    }

    void forward(int speed) const {
        digitalWrite(coastPin, HIGH);
        digitalWrite(brakePin, LOW);
        front.forward(speed);
        back.forward(speed);
    }

    void backward(int speed) const {
        digitalWrite(coastPin, HIGH);
        digitalWrite(brakePin, LOW);
        front.backward(speed);
        back.backward(speed);
    }
};
struct Car {
    Driver left;
    Driver right;

    Car()
            : left(Wheel(2, 3), Wheel(4, 5), 15, 14),
              right(Wheel(6, 7), Wheel(10, 11), 19, 16) {}

    void init() const {
        left.init();
        right.init();
    }

    void forward() {
        left.forward(0);
        right.forward(0);
    }
};
struct Led {
    int r_pin;
    int g_pin;
    int b_pin;

    Led(int rt, int gt, int bt) : r_pin(rt), g_pin(gt), b_pin(bt) {}

    void init() const {
        pinMode(r_pin, OUTPUT);
        pinMode(g_pin, OUTPUT);
        pinMode(b_pin, OUTPUT);
    }

    void setColor(int r, int g, int b) const {
        analogWrite(r_pin, r);
        analogWrite(g_pin, g);
        analogWrite(b_pin, b);
    }
};

Led status = Led(23, 22, 13);
Car mouse;

void setup() {
    Serial.begin(115200);
    mouse.init();

    Serial.println("Setup complete!");

    status.init();
    status.setColor(0, 0, 0);
    if (!driver.init())
        Serial.println("Rx/Tx init failed");

    pinMode(RX_PIN, INPUT); // Set RX_PIN as INPUT
}

unsigned long ledOnTime = 0;
const unsigned long ledDuration = 500; // in milliseconds
void loop() {
    mouse.forward();
//    delay(100);
//    Serial.println(sensor.measureDistanceCm());

    // Check if it's time to transmit the message
//    unsigned long currentTime = millis();
//    if (currentTime - lastTransmissionTime >= transmissionInterval) {
//        lastTransmissionTime = currentTime;
//
//        // Transmit the message
//        char *msg = "Ping!";
//        driver.send((uint8_t *)msg, strlen(msg));
//        driver.waitPacketSent();
//
//        status.setColor(255, 0, 0); // Set LED color to indicate transmission
//    }

    uint8_t buf[12];
    uint8_t buflen = sizeof(buf);
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
        int i;
        // Message with a good checksum received, dump it.
        Serial.print("Received: ");
        Serial.println((char*)buf);

        status.setColor(0, 0, 255); // Turn on LED
        ledOnTime = millis(); // Store the current time
    }

// Check if it's time to turn off the LED
    if (millis() - ledOnTime >= ledDuration && ledOnTime > 0) {
        status.setColor(0, 0, 0); // Turn off LED
        ledOnTime = 0; // Reset the timer
    }
}