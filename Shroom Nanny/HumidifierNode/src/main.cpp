#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AccelStepper.h>

const char* ssid = "SkyeNet";
const char* password = "ernorubiks";
const int port = 80; // Port number should match the server's
WiFiClient nanny;

// Static IP details
IPAddress local_IP(192, 168, 0, 232); // Example IP address, change as needed
IPAddress nanny_IP(192, 168, 0, 231);
IPAddress gateway(192, 168, 1, 1);    // Typically the address of your router
IPAddress subnet(255, 255, 255, 0);

//Our humidifier is controlled by a four wire stepper motor glued to the knob lol
struct Knob{
    char target[2] = {'0', '0'};

    int currentPercentage = 0;
    int startPos = 0;

    //Stepper code below here
    bool isOn = false; //This is for when the humidifier is turned off by a user
    bool moving = false;

    const int stepsToMax = 1350; //This is how many steps our stepper can take AFTER turning on
    AccelStepper stepper = *new AccelStepper(1, D2, D3);

    //The lights pin can't be trusted to be consistently measured as on,
    // so we need to give it a few chances to get flagged
    const int lightsOffChances = 50;
    bool isRedLightOn() const {
        for (int i = 0; i < lightsOffChances; i++)
            if (analogRead(D0) > 2000)
                return true;

        return false;
    }
    bool isGreenLightOn() const {
        for (int i = 0; i < lightsOffChances; i++)
            if (analogRead(D1) > 2000)
                return true;

        return false;
    }

    void reset(bool turnOff = false) {
        Serial.println("Resetting humidifier...");

        String returnMessage = "humidifier_reset";
        if (turnOff)
            returnMessage = "humidifier_off";

        if (!isRedLightOn() && !isGreenLightOn()) {
            Serial.println("Humidifier is already off.");
            nanny.println(returnMessage);
            return;
        }

        stepper.moveTo(stepper.currentPosition() - (stepsToMax + 1000));
        while (stepper.distanceToGo() != 0 && (isRedLightOn() || isGreenLightOn()))
            stepper.run();

        Serial.println("Min position reached.");
        stepper.stop();
        stepper.setCurrentPosition(0);

        isOn = false;

        nanny.println(returnMessage);
    }

    void turnOn() {
        if (isRedLightOn() && isGreenLightOn()) {
            Serial.println("Humidifier is already on.");
            nanny.println("humidifier_on");
            return;
        }

        Serial.println("Turning on humidifier...");
        stepper.moveTo(stepper.currentPosition() + (stepsToMax + 1000));
        while (stepper.distanceToGo() != 0 && !isRedLightOn() && !isGreenLightOn()) {
            stepper.run();
        }

        stepper.stop();
        stepper.setCurrentPosition(stepper.currentPosition());

        isOn = true;
        startPos = stepper.currentPosition();

        nanny.println("humidifier_on");
    }

    void setByPercentage(int percentage) {
        if (!isOn || percentage == currentPercentage || percentage < 0 || percentage > 100 || moving)
            return;

        moving = true;

        if (!isRedLightOn() && !isGreenLightOn())
            return reset();

        int onePercent = (stepsToMax - startPos) / 100;
        int percTarget = (onePercent * percentage) + startPos;

        stepper.moveTo(percTarget);

        Serial.println("Set by percentage to: " + String(percentage) + "% @ " + percTarget + " steps");
        nanny.println("at_percentage:" + String(percentage));

        currentPercentage = percentage;
    }
};
Knob knob;

void setup() {
    Serial.begin(115200);

    // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet))
        Serial.println("STA Failed to configure");

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFiClass::status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    float speed = 2000; // Steps per second
    knob.stepper.setMaxSpeed(speed); // Set the maximum steps per second
    knob.stepper.setSpeed(speed); // Set the initial speed
    knob.stepper.setAcceleration(1000); // Set the acceleration
//    knob.reset();

    //Light sensor pins
    pinMode(D0, INPUT);
    pinMode(D1, INPUT);

    //Stepper outputs
    pinMode(D8, OUTPUT);
    pinMode(D9, OUTPUT);
    pinMode(D10, OUTPUT);

    //Attempt to connect to our
    if (nanny.connect(nanny_IP, port))
        nanny.println("ready");
}

unsigned long lastVerificationTime = 0;
unsigned long lastVerificationAttempt = millis();
const unsigned long verificationInterval = 5000; // 5 seconds
int dropped = 0;
int maxDropped = 4;
bool verifyServerConnection() {

    // Check if verification interval has passed
    if (millis() - lastVerificationTime < verificationInterval)
        return true;

    if (dropped > maxDropped) {
        Serial.println("Server connection failed to validate");
        dropped = 0;

        return false;
    }

    //If we've missed less than our dropped amount, return true anyway. The other end might be processing stuff.
    if (millis() - lastVerificationAttempt > 5000) {
        lastVerificationAttempt = millis();
        nanny.println("ping");
        dropped++;
    }

    return true;
}

bool needsWater = false;
void loop() {
    // Check if the nanny is connected to the server
    if (!nanny.connected() || !verifyServerConnection() || dropped > maxDropped) {
        Serial.println("Disconnect reason: " + String(nanny.connected()) + "|" + String(verifyServerConnection()) + "|" + String(dropped > maxDropped));
        Serial.println("Client disconnected. Attempting to reconnect...");
        nanny.stop(); // Ensure the previous connection is closed cleanly

        // Attempt to reconnect to the server
        if (nanny.connect(nanny_IP, port)) {
            Serial.println("Connected to server, sending ready message");
            nanny.println("ready");
        } else {
            Serial.println("Reconnection failed. Retrying in 5sec...");
            delay(5000); // Wait for 5 seconds before retrying
        }
    }

    if (nanny.available()) {
        String message = nanny.readStringUntil('\n');
        message.trim();

        if (!message.equals("pong"))
            Serial.println("From server: " + message);

        if (message.equals("reset") || message.equals("turn_off")) {
            knob.reset(message.equals("turn_off"));

        } else if (message.equals("turn_on")) {
            knob.turnOn();

        } else if (message.startsWith("set_percentage:")) {
            String percentageStr = message.substring(strlen("set_percentage:"));
            int percentage = percentageStr.toInt();
            Serial.println("Setting percentage to " + String(percentage) + "%.");
            knob.setByPercentage(percentage);

        } else if (message.equals("report_status")) {
            /*
             * This gets called when the humidifier first connects to the nanny
             * We should report the humidifiers current status here to avoid resetting
             */

            delay(500);

            //If our humidifier is off, re-initialize our knob so the nanny knows we're ready
            if (!knob.isOn)
                knob.reset();
            else
                nanny.println("at_percentage:" + String(knob.currentPercentage));
        }

        //We send unhandled ping messages back and forth to get here and manage verification
        //Getting a message from the server counts as verification
        lastVerificationTime = millis();
        dropped = 0;
    }

    //If we need water then say that endlessly till water is given and humidifier is reset
    if (knob.isOn && !knob.isGreenLightOn() && knob.isRedLightOn())
        nanny.println("needs_water");

    if (knob.stepper.distanceToGo() != 0)
        knob.stepper.run();
    else knob.moving = false;
}


