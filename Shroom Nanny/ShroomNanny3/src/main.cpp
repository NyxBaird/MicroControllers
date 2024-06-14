#include <Arduino.h>
#include <TFT_eSPI.h>

#include <WiFi.h>
#include <IRremote.hpp>
#include <WiFiClient.h>
#include <cmath>

#include <Adafruit_Sensor.h>
#include <DHT.h>

/*
 * SYSTEM
 */
String version = "4.1";
TFT_eSPI tft = TFT_eSPI();


/*
 * INPUTS
 */
DHT onboard_sensor(25, DHT11);
DHT offboard_sensor(26, DHT22);
float lastF = 0;
float lastH = 0;
float lastF2 = 0;
float lastH2 = 0;

const int Button0 = 0;
const int Button1 = 35;


/*
 * WIFI
 */
const char* ssid = "SkyeNet";
const char* password = "ernorubiks";

// Static IP details
IPAddress local_IP(192, 168, 0, 231);
IPAddress gateway(192, 168, 1, 1);    // Typically the router
IPAddress subnet(255, 255, 255, 0);


/*
 * SERVER
 */
//Used to acknowledge reception of commands
char* ACKNOWLEDGED = "ack";

WiFiServer server(80);

struct Node {
    WiFiClient client;
    IPAddress IP;

    explicit Node(const IPAddress& ip)
            : IP(ip) {};
};


/*
 * Humidifier
 */
struct Humidifier {
    Node node;

    Humidifier()
        : node(IPAddress(192, 168, 0, 232)) {};

    bool isOn = false;

    //Set target humidity stuff
    int targetInt = 88; //This is the actual var we use to set the target humidity

    int currentTargetIndex = 0; //The index in target[] the user is currently setting
    char target[2] = {'0', '0'};

    int currentPercentage = 0;

    bool settingTarget = false;




    void toggleSettingTarget() {
        settingTarget = !settingTarget;
    }
    void setByPercentage(int to) {

    }

    //This is for setting the target on screen with a remote. "Pieces" are each number as it's entered.
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

    void togglePower() {
        if (isOn)
            sendCommand("turn_off");
        else
            sendCommand("turn_on");
    }

    bool sendCommand(const String& command) {
        if (!node.client || !node.client.connected())
            return false;

        node.client.println(command);
        Serial.println("Command sent to humidifier: " + command);

        // Wait for the client's response
        unsigned long startTime = millis();
        while (node.client.connected() && !node.client.available()) {
            if (millis() - startTime > 5000) { // 5 second timeout for the client to respond
                Serial.println("Response timeout.");
                break;
            }
            delay(10); // Small delay to prevent busy waiting
        }

        return node.client.available() && node.client.readStringUntil('\n') == ACKNOWLEDGED;
    }
};
Humidifier humidifier;


struct Fan {
    static void begin() {
        pinMode(33, OUTPUT);
    }

    static void on() {
        digitalWrite(33, HIGH);
    }

    static void off() {
        digitalWrite(33, LOW);
    }
};
Fan fan;

//If alert ever contains anything we'll display it as a temporary alert.
String alert = "";

void drawScreen(const String& message = "");
void drawScreen(const String& message) {
    if (alert != "") {
        //Draw our background
        tft.fillScreen(TFT_BLACK);

        //Draw IP and version
        tft.setTextColor(TFT_WHITE);

        tft.drawString(alert, 3, 3, 2);
        delay(2000);
        alert = "";
    }

    Serial.println("Drawing screen...");

    //Draw our background
    tft.fillScreen(TFT_BLACK);

    //Draw IP and version
    tft.setTextColor(TFT_WHITE);
    tft.drawString("IP: " + WiFi.localIP().toString(), 3, 126, 1);
    tft.drawString("v" + version, 210, 126, 1);

    tft.setTextColor(TFT_BLUE);
    tft.drawString("Room:", 10, 10, 4);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(String(lastF, 0) + "F - " + String(lastH, 0) + "%", 120, 10, 4);
    tft.setTextColor(TFT_GREEN);
    tft.drawString("Tent:", 10, 38, 4);
    tft.setTextColor(TFT_WHITE);
    tft.drawString(String(lastF2, 0) + "F - " + String(lastH2, 0) + "%", 120, 38, 4);


    if (humidifier.settingTarget) {
        tft.setTextColor(TFT_RED);
        tft.drawString("Set New Goal: " + String(humidifier.target[0]) + String(humidifier.target[1]), 10, 70, 4);
    } else {
        tft.setTextColor(TFT_CYAN);
        tft.drawString("Target", 30, 84, 1);
        tft.drawString("Humidity", 25, 91, 1);
        tft.drawString(": " + String(humidifier.targetInt) + "%", 75, 79, 4);

        tft.setTextColor(TFT_WHITE);
        tft.setRotation(tft.getRotation() - 1);
        tft.drawString("Stepper", 21, 149, 1);
        tft.setRotation(tft.getRotation() + 1);

        String status = String(humidifier.currentPercentage) + "%";
        if (!humidifier.isOn) {
            tft.setTextColor(TFT_RED);
            status = "Off";
        }

        tft.drawString(": " + status, 165, 80, 4);
    }


    //If no message was sent and we're redrawing the screen assume we're loading
    if (message == "Loading...") {
        tft.setTextColor(TFT_RED);
        tft.drawString(message, 65, 58, 4);
        return;

        //If our connection status reads as disconnected...
    } else if (WiFiClass::status() != WL_CONNECTED) {
        tft.setTextColor(TFT_RED);
        tft.drawString("DISCONNECTED!", 3, 115, 1);
    }
}

//Handles IR control signals
void runIrFunc(unsigned long cmd) {
    Serial.println("IR command: " + String(cmd, HEX));

    switch(cmd) {
        case 0xba45ff00: //Power btn
            humidifier.togglePower();
            drawScreen();
            break;
        case 0xb847ff00: //Func btn (set target humidity)
            humidifier.toggleSettingTarget();
            drawScreen();
            break;
        case 0xea15ff00: //Vol down btn
            break;
        case 0xbb44ff00: //Back btn
            if (humidifier.settingTarget)
                humidifier.currentTargetIndex = 0;
            else
                humidifier.setByPercentage(0);
            break;
        case 0xb946ff00: //Vol up btn
            break;
        case 0xbc43ff00: //Next btn
            if (humidifier.settingTarget) {
                alert = "New Target: " + String(humidifier.targetInt) + "%";
                humidifier.settingTarget = false;
                drawScreen();
            } else
                humidifier.setByPercentage(100);
            break;
        case 0xe916ff00: //0
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('0');
                drawScreen();
            } else
                humidifier.setByPercentage(0);
            break;
        case 0xf30cff00: //1
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('1');
                drawScreen();
            } else
                humidifier.setByPercentage(10);
            break;
        case 0xe718ff00: //2
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('2');
                drawScreen();
            } else
                humidifier.setByPercentage(20);
            break;
        case 0xa15eff00: //3
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('3');
                drawScreen();
            } else
                humidifier.setByPercentage(30);
            break;
        case 0xf708ff00: //4
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('4');
                drawScreen();
            } else
                humidifier.setByPercentage(40);
            break;
        case 0xe31cff00: //5
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('5');
                drawScreen();
            } else
                humidifier.setByPercentage(50);
            break;
        case 0xa55aff00: //6
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('6');
                drawScreen();
            } else
                humidifier.setByPercentage(60);
            break;
        case 0xbd42ff00: //7
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('7');
                drawScreen();
            } else
                humidifier.setByPercentage(70);
            break;
        case 0xad52ff00: //8
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('8');
                drawScreen();
            } else
                humidifier.setByPercentage(80);
            break;
        case 0xb54aff00: //9
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('9');
                drawScreen();
            } else
                humidifier.setByPercentage(90);
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing ShroomNanny v" + version);

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

    server.begin(); // Start TCP server

    //Init the screen
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Initializing", 89, 45, 2);
    tft.drawString("ShroomNanny", 62, 65, 2);
    tft.drawString("v" + version, 150, 71, 1);


    //Start our DHT11's
    onboard_sensor.begin();
    offboard_sensor.begin();

    //Start our fan
    Fan::begin();
    Fan::on();

    //Init the IR receiver
    IrReceiver.begin(38, ENABLE_LED_FEEDBACK);

    drawScreen();
}

bool hitTarget = false;
void loop() {
    bool refreshScreen = false;

    WiFiClient client = server.available(); //Listen for incoming clients
    if (client) {
        IPAddress clientIP = client.remoteIP();
        Serial.print("New Client: ");
        Serial.println(clientIP.toString()); // Print the client's IP address

        if (client.remoteIP() == humidifier.node.IP)
            humidifier.node.client = client;
    }

    if (humidifier.node.client && humidifier.node.client.connected()) {

        //If a message is available from the humidifier process that first
        if (humidifier.node.client.available()) {
            // Read data from the client
            String message = humidifier.node.client.readStringUntil('\n');
            Serial.println("Received: " + message);
        }

        //Read our onboard sensor
        float h = onboard_sensor.readHumidity();
        float f = onboard_sensor.readTemperature(true);
        if (isnan(h) || isnan(f)) {
            //        throwError("CTRL reading failure!");
            //        return;
        } else {
            if (f != lastF || h != lastH) {
                lastF = f;
                lastH = h;
                Serial.println("Onboard - T: " + String(f) + " | H: " + String(h));
                refreshScreen = true;
            }
        }

        //Read our offboard sensor
        float h2 = offboard_sensor.readHumidity();
        float f2 = offboard_sensor.readTemperature(true);
        if (isnan(h2) || isnan(f2)) {
            //        printError("AUX reading failure!");
            //        return;
        } else {
            if (f2 != lastF2 || h2 != lastH2) {
                lastF2 = f2;
                lastH2 = h2;
                Serial.println("Offboard - T: " + String(f2) + " | H: " + String(h2));
                refreshScreen = true;
            }
        }

        //Automate our humidifer based on existing parameters
        if (!humidifier.settingTarget && humidifier.targetInt > 0 && humidifier.node.client.connected()) {
            int upperBuffer = 2;

            if (hitTarget && h2 < humidifier.targetInt)
                hitTarget = false;

            if (!hitTarget && h2 >= humidifier.targetInt + upperBuffer)
                hitTarget = true;

            if (hitTarget) {
                humidifier.setByPercentage(0);
            } else {
//            int percentage = 100 - h2 - (100 - humidifier.targetInt);

                //Give us a buffer above our target before we turn off completely
                int percentage = 100;
                if (h2 >= humidifier.targetInt && h2 < humidifier.targetInt + upperBuffer)
                    percentage = 1;

                humidifier.setByPercentage(percentage);
            }

//        if (h2 < humidifier.targetInt - 20)
//            fan.set(80);
//        else if (h2 < humidifier.targetInt)
//            fan.set(90);
//        else
//            fan.set(100);
        }
    } else {
        if (humidifier.node.client) {
            humidifier.node.client.stop(); // Free up resources by stopping the client
        }
    }



    //Read our onboard sensor
    float h = onboard_sensor.readHumidity();
    float f = onboard_sensor.readTemperature(true);
    if (isnan(h) || isnan(f)) {
        //        throwError("CTRL reading failure!");
        //        return;
    } else {
        if (f != lastF || h != lastH) {
            lastF = f;
            lastH = h;
            Serial.println("Onboard - T: " + String(f) + " | H: " + String(h));
            refreshScreen = true;
        }
    }

    //Read our offboard sensor
    float h2 = offboard_sensor.readHumidity();
    float f2 = offboard_sensor.readTemperature(true);
    if (isnan(h2) || isnan(f2)) {
        //        printError("AUX reading failure!");
        //        return;
    } else {
        if (f2 != lastF2 || h2 != lastH2) {
            lastF2 = f2;
            lastH2 = h2;
            Serial.println("Offboard - T: " + String(f2) + " | H: " + String(h2));
            refreshScreen = true;
        }
    }

    //Catch remote commands
    if (IrReceiver.decode()) {
        Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
        runIrFunc(IrReceiver.decodedIRData.decodedRawData);
        IrReceiver.resume();
    }


    if (refreshScreen)
        drawScreen();
}