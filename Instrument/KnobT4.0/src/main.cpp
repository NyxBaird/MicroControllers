#include <Arduino.h>
#include "SimpleFOC.h"
#include "Wire.h"

BLDCMotor motor = BLDCMotor(4);
BLDCDriver3PWM driver = BLDCDriver3PWM(2, 3, 4, 5);
MagneticSensorI2C sensor = MagneticSensorI2C(AS5600_I2C);

// Define PID parameters and voltage limit
#define FOC_PID_P 4
#define FOC_PID_I 0
#define FOC_PID_D 0.04
#define FOC_PID_OUTPUT_RAMP 10000
#define FOC_PID_LIMIT 10


#define btnPin 7

void setup() {
    Serial.begin(115200);
    Serial.println("Hello World");

    pinMode(btnPin, INPUT_PULLUP);

    Wire.begin();

    driver.voltage_power_supply = 8;
    driver.voltage_limit = 8;

    driver.init();
    sensor.init();

    motor.linkDriver(&driver);
    // motor.linkSensor(&sensor);

    motor.controller = MotionControlType::velocity_openloop;

    motor.PID_velocity.P = FOC_PID_P;
    motor.PID_velocity.I = FOC_PID_I;
    motor.PID_velocity.D = FOC_PID_D;
    motor.PID_velocity.output_ramp = FOC_PID_OUTPUT_RAMP;
    motor.PID_velocity.limit = FOC_PID_LIMIT;

    // motor.controller = MotionControlType::angle;

    motor.init();
    // motor.initFOC();
}

float targetAngle = 0;
bool btnPrevPressed = false;
void loop() {
    // Set a small target velocity for slow rotation
    float targetVelocity = 1.0; // Adjust this value for desired speed

    // Execute FOC algorithm
    // motor.loopFOC();

    // Move the motor at the target velocity
    motor.move(targetVelocity);


    // bool btnPressed = (digitalRead(btnPin) == LOW);
    //
    // if (btnPressed && !btnPrevPressed) {
    //     Serial.print("Pressed Btn");
    //     targetAngle = PI;
    // }
    //
    // motor.move(targetAngle);
    // motor.loopFOC();
    // delay(100);
}