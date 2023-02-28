#include <Arduino.h>

#include "IRremote.h"
IRrecv irrecv(A0);
decode_results results;

#include <Stepper.h>
struct Humidifier{

    //Set target functions
    int targetInt = 0;
    int currentTargetIndex = 0;
    char target[2] = {'0', '0'};

    bool settingTarget = false;
    void toggleSettingTarget() {
        settingTarget = !settingTarget;
    }

    void setTargetPieces(char piece) {
        target[currentTargetIndex] = piece;

        if (currentTargetIndex == 0)
            currentTargetIndex++;
        else {
            String targetString = String(target[0]) + String(target[1]);
            currentTargetIndex = 0;
            targetInt = (int)targetString.toInt();
            Serial.println("Target set to " + String(targetInt) + "%.");
        }
    }


    //Stepper code below here
    bool isOn = false; //This is for when the humidifier is turned off by a user
    bool softOff = false; //This is for when the humidifier is turned off by the software

    int stepsPerRevolution = 2048; //This is how many steps our stepper can take
    int stepsToPowerOn = 245; //This is how many steps it takes to click the humidifier on or off
    int stepsToMax = 1653; //This is how many steps it takes to max out the humidifier AFTER it's been turned on
    int currentStep = 0;

    Stepper stepper = Stepper(stepsPerRevolution, 4, 6, 5, 7);

    void reset() {
        stepper.step(-stepsPerRevolution);
        currentStep = 0;
        isOn = false;
    }
    void togglePower() {
        if (isOn) {
            if (currentStep)
                repositionBy(-currentStep);

            stepper.step(-stepsToPowerOn);
            isOn = false;
        } else {
            stepper.step(stepsToPowerOn);
            currentStep = 0;
            isOn = true;
        }
    }

    void repositionBy(int steps) {
        if (isOn) {
            if (currentStep + steps > stepsToMax)
                steps = stepsToMax - currentStep;
            else if (currentStep + steps < 0)
                steps = -currentStep;

            Serial.println("Stepping by " + String(steps) + " steps.");

            currentStep += steps;

            stepper.step(steps);
        }
    }
    void setByPercentage(int percentage) {
        if (isOn) {
            if (percentage > 100)
                percentage = 100;
            else if (percentage < 0)
                percentage = 0;

            if (softOff && percentage > 0) {
                stepper.step(stepsToPowerOn);
                softOff = false;
            }

            int piece = stepsToMax / 100;
            int steps = -(currentStep - (piece * percentage));

            Serial.println("Stepping to " + String(percentage) + "% (" + String(steps) + " steps).");
            Serial.println("Current step: " + String(currentStep) + " steps.");


            repositionBy(steps);

            if (percentage == 0) {
                stepper.step(-stepsToPowerOn);
                softOff = true;
            }
        }
    }
};
Humidifier humidifier;


#include <Adafruit_Sensor.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

#include <DHT.h>
DHT onboard_sensor(2, DHT11);
DHT offboard_sensor(3, DHT11);

void drawClimateData(float h, float f, float h2, float f2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("R:");
    lcd.print((int) f);
    lcd.print((char) 223);
    lcd.print((int) h);
    lcd.print("% GOL:");
    lcd.print(humidifier.targetInt);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("M:");
    lcd.print((int) f2);
    lcd.print((char) 223);
    lcd.print((int) h2);
    lcd.print("% SET:");
    lcd.print((int) (humidifier.currentStep / (float) humidifier.stepsToMax * 100));
    lcd.print("%");
}
void drawGetTarget() {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("Set target: ");
    lcd.print(humidifier.target[0]);
    lcd.print(humidifier.target[1]);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("Nav with Bck/Fwd");
}


void printError(String error) {
    Serial.println("Error: " + error);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR: ");
    lcd.setCursor(0, 1);
    lcd.print(error);
}

void runIrFunc(unsigned long cmd) {

//    Serial.println("IR command: " + String(cmd, HEX));

    switch(cmd) {
        case 0xFFA25D: //Power btn
            humidifier.togglePower();
            break;
        case 0xFFE21D: //Func btn (set target humidity)
            humidifier.toggleSettingTarget();
            break;
        case 0xFFA857: //Vol down btn
            break;
        case 0xFF22DD: //Back btn
            if (humidifier.settingTarget)
                humidifier.currentTargetIndex = 0;
            else
                humidifier.setByPercentage(0);
            break;
        case 0xFF629D: //Vol up btn
            break;
        case 0xFFC23D: //Next btn
            if (humidifier.settingTarget) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("New Target: " + String(humidifier.targetInt) + "%");
                delay(1500);

                humidifier.settingTarget = false;
            } else
                humidifier.setByPercentage(100);
            break;
        case 0xff6897: //0
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('0');
            else
                humidifier.setByPercentage(0);
            break;
        case 0xff30cf: //1
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('1');
            else
                humidifier.setByPercentage(10);
            break;
        case 0xff18e7: //2
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('2');
            else
                humidifier.setByPercentage(20);
            break;
        case 0xff7a85: //3
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('3');
            else
                humidifier.setByPercentage(30);
            break;
        case 0xff10ef: //4
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('4');
            else
                humidifier.setByPercentage(40);
            break;
        case 0xff38c7: //5
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('5');
            else
                humidifier.setByPercentage(50);
            break;
        case 0xff5aa5: //6
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('6');
            else
                humidifier.setByPercentage(60);
            break;
        case 0xff42bd: //7
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('7');
            else
                humidifier.setByPercentage(70);
            break;
        case 0xff4ab5: //8
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('8');
            else
                humidifier.setByPercentage(80);
            break;
        case 0xff52ad: //9
            if (humidifier.settingTarget)
                humidifier.setTargetPieces('9');
            else
                humidifier.setByPercentage(90);
            break;
//        default:
//            printError("Invalid IR command!");
    }
}

void setup() {
    lcd.begin(16, 2);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");

    humidifier.stepper.setSpeed(10);
    humidifier.reset();

    Serial.begin(9600);
    Serial.println("hello!");

    onboard_sensor.begin();
    offboard_sensor.begin();

    irrecv.enableIRIn();
}

void loop() {
    float h = onboard_sensor.readHumidity();
    float f = onboard_sensor.readTemperature(true);
    if (isnan(h) || isnan(f)) {
        printError("CTRL reading failure!");
        return;
    }

    float h2 = offboard_sensor.readHumidity();
    float f2 = offboard_sensor.readTemperature(true);
    if (isnan(h2) || isnan(f2)) {
        printError("AUX reading failure!");
        return;
    }

    if (humidifier.settingTarget)
        drawGetTarget();
    else
        drawClimateData(h, f, h2, f2);

    //Catch remote commands
    if (irrecv.decode(&results)) {
        runIrFunc(results.value);
        irrecv.resume(); // Receive the next value
    }

    if (!humidifier.settingTarget && humidifier.targetInt > 0 && humidifier.isOn) {
        int percentage = 100 - h2 - (100 - humidifier.targetInt);

        if (h2 < humidifier.targetInt - 5)
            percentage = 100;

        humidifier.setByPercentage(percentage);
    }

    delay(100);
}