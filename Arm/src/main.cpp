#include <Arduino.h>
#include <AccelStepper.h>
#include "IRremote.hpp"

struct Joint {
    int en_pin;
    int min_pin;
    int max_pin;

    int maxSteps;

    int speed = 4000;

    bool initComplete = false;

    // Initialize the stepper library on pins 2 and 3 for direction and steps
    AccelStepper stepper;
    AccelStepper* stepper2 = nullptr; // Pointer for optional second stepper

    Joint(int enable_pin, int direction_pin, int steps_pin, int min_pin, int max_pin,
          int direction_pin2 = -1, int steps_pin2 = -1)
            : en_pin(enable_pin), min_pin(min_pin), max_pin(max_pin), stepper(1, steps_pin, direction_pin) {

        if (direction_pin2 != -1 && steps_pin2 != -1) {
            stepper2 = new AccelStepper(1, steps_pin2, direction_pin2);
        }
    }
    void init() {
        pinMode(en_pin, OUTPUT);
        digitalWrite(en_pin, LOW); // Assuming LOW enables the driver

        pinMode(min_pin, INPUT); //init our minimum pin
        pinMode(max_pin, INPUT); //init our maximum pin

        stepper.setMaxSpeed(speed); // Set the maximum steps per second
        stepper.setSpeed(speed); // Set the initial speed
        stepper.setAcceleration(3200); // Set the acceleration

        // Conditionally initialize the second stepper
        if (stepper2) {
            stepper2->setMaxSpeed(speed);
            stepper2->setSpeed(speed);
            stepper2->setAcceleration(3200);
        }

        zero();
        max();
        initComplete = true;
    }

    void zero() {
        Serial.println("Zeroing... Init'd? " + String(initComplete));

        // Move the stepper in the negative direction until the min sensor is triggered
        if (initComplete)
            stepper.moveTo(0);
        else
            stepper.moveTo(stepper.currentPosition() - 50000); // Adjust as needed

        // If second stepper exists, move it in the opposite direction
        if (stepper2) {
            if (initComplete)
                stepper.moveTo(0);
            else
                stepper2->moveTo(stepper2->currentPosition() + 50000);
        }

        // Run both steppers if the second one exists
        while ((stepper.distanceToGo() != 0 || (stepper2 && stepper2->distanceToGo() != 0)) && digitalRead(min_pin) == HIGH) {
            stepper.run();
            Serial.print("Current Position: ");
            Serial.print(stepper.currentPosition());
            Serial.print(", Distance to Go: ");
            Serial.println(stepper.distanceToGo());
            Serial.print(", Min Pin State: ");
            Serial.println(digitalRead(min_pin));

            if (stepper2) {
                stepper2->run();
            }
        }


        Serial.println("Min position reached.");
        stepper.stop();
        stepper.setCurrentPosition(0);

        Serial.println("Max: " + String(digitalRead(max_pin)) + " | Min: " + String(digitalRead(min_pin)));
        Serial.println("Finished Zeroing!");
    }

    void max() {
        Serial.println("Maxing... Init'd? " + String(initComplete));

        if (initComplete)
            stepper.moveTo(maxSteps);
        else
            stepper.moveTo(stepper.currentPosition() + 50000);

        if (stepper2) {
            if (initComplete)
                stepper.moveTo(maxSteps);
            else
                stepper2->moveTo(stepper2->currentPosition() - 50000);
        }

        while(stepper.distanceToGo() != 0 && digitalRead(max_pin) == HIGH)
            stepper.run();


        stepper.stop();
        maxSteps = stepper.currentPosition();

        Serial.println("Max: " + String(digitalRead(max_pin)) + " | Min: " + String(digitalRead(min_pin)));
        Serial.println("Finished Maxing! Max: " + String(maxSteps));
    }
};
struct Arm {
    Joint base;
    Joint joint1;

    Arm() : base(1, 2, 3, 12, 10),
            joint1(1, 1, 4, 12, 10, 5, 6) {}

    void init () {
//        if (!base.initComplete)
        base.init();
//        joint1.init();

        Serial.println("Base stepper at " + String(base.stepper.currentPosition()));
    }

    void loop() {
        if (base.stepper.distanceToGo() != 0) { // Check if the stepper has reached the target
            base.stepper.run(); // Move the stepper towards the target
        }
    }
};
Arm arm;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.begin(115200);
    Serial.println("Initializing...");

    //Initialize all steppers and return them to zero
//    arm.init();

    IrReceiver.begin(11, ENABLE_LED_FEEDBACK);

    Serial.println("Initialized!");
    digitalWrite(LED_BUILTIN, LOW);
}

void runIrFunc(unsigned long cmd) {
    Serial.println("IR command: " + String(cmd, HEX));
    digitalWrite(LED_BUILTIN, HIGH);

    int num = 0;

    switch(cmd) {
        case 0xba45ff00: //Power btn
            break;
        case 0xb847ff00: //Func btn (set target humidity)
            break;
        case 0xea15ff00: //Vol down btn
            break;
        case 0xbb44ff00: //Back btn
            break;
        case 0xb946ff00: //Vol up btn
            break;
        case 0xbc43ff00: //Next btn
            break;
        case 0xBF40FF00: // Play/Pause btn
            arm.init();
            break;
        case 0xe916ff00: //0
            arm.base.zero();
            break;
        case 0xf30cff00: //1
            num = 1;
            break;
        case 0xe718ff00: //2
            num = 2;
            break;
        case 0xa15eff00: //3
            num = 3;
            break;
        case 0xf708ff00: //4
            num = 4;
            break;
        case 0xe31cff00: //5
            num = 5;
            break;
        case 0xa55aff00: //6
            num = 6;
            break;
        case 0xbd42ff00: //7
            num = 7;
            break;
        case 0xad52ff00: //8
            num = 8;
            break;
        case 0xb54aff00: //9
            arm.base.max();
            break;
    }

    if (num > 0) {
        int newPos = arm.joint1.stepper.currentPosition() + (num * 1000);
        int newPos2 = 0;
        if (arm.joint1.stepper2)
            newPos2 = arm.joint1.stepper2->currentPosition() - (num * 1000);

        Serial.println("Moving arm to; " + String(newPos));
        arm.joint1.stepper.moveTo(newPos);
        if (arm.joint1.stepper2)
            arm.joint1.stepper2->moveTo(newPos2);

        while ((arm.joint1.stepper.distanceToGo() != 0 || (arm.joint1.stepper2 && arm.joint1.stepper2->distanceToGo() != 0))) {
            arm.joint1.stepper.run();
            arm.joint1.stepper2->run();
        }
    }

    digitalWrite(LED_BUILTIN, LOW);
}

unsigned long lastMessageTime = 0;
void loop() {
    unsigned long currentTime = millis();

    //Catch remote commands
    if (IrReceiver.decode()) {
        runIrFunc(IrReceiver.decodedIRData.decodedRawData);
        IrReceiver.resume();
    }

    if (currentTime - lastMessageTime >= 2000) {
        Serial.println("Max: " + String(digitalRead(arm.base.max_pin)) + " | Min: " + String(digitalRead(arm.base.min_pin)));
        lastMessageTime = currentTime;
    }

    arm.loop();
}