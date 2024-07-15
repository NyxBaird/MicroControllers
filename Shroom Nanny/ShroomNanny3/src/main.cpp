#include <cmath>
#include <ctime>

#include <Arduino.h>
#include <TFT_eSPI.h>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <HttpClient.h>
#include <ArduinoJson.h>

#include <IRremote.hpp>

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "Wire.h"
#include "SHT31.h"
#include <FastLED.h>
#include "Mush.h"

#include <uRTClib.h>

#define WIREADDR 0x44
#define TCAADDR 0x70
int tcaAssigned = 0; //Our count of tca ports assigned

#define NUM_LEDS 3
CRGB leds[NUM_LEDS];

#define PHOTORESISTOR 39


/*
 * SYSTEM
 */
String version = "4.1";
unsigned long currentMillis;
static void tcaSelect(uint8_t i) {
    //Our second tca is actually wired into our 7th i2c port
    if (i == 2)
        i = 7;

    if (i > 7) return;

    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
} //Selects the current TCA i2c port


/*
 * WiFi Settings
 */
const char* ssid = "SkyeNet";
const char* password = "ernorubiks";
IPAddress local_IP(192, 168, 0, 231);
IPAddress gateway(192, 168, 0, 1);    // Typically the router
IPAddress subnet(255, 255, 255, 0);


/*
 * Communicate with NyxB.me
 */
struct WebServer {
    bool online = false;

    const char* domain;
    WiFiClientSecure wifiClient;
    HttpClient client;

    explicit WebServer(const char* server) : domain(server), client(wifiClient, server, 443) {} // Use port 443 for HTTPS

    void sendPost(const char* uri, const DynamicJsonDocument* data = nullptr) {
        String output;
        if (data) {
            serializeJson(*data, output); // Serialize provided data into a string
        } else {
            DynamicJsonDocument emptyDoc(1024);
            serializeJson(emptyDoc, output); // Serialize an empty JSON object
        }

        if (!wifiClient.connect(domain, 443)) {
            Serial.println("Connection failed");
            return;
        }

        // Start the request
        client.beginRequest();
        client.post(uri);

        // Send headers
        client.sendHeader(HTTP_HEADER_CONTENT_TYPE, "application/json");
        client.sendHeader(HTTP_HEADER_CONTENT_LENGTH, output.length());

        // Send the body
        client.beginBody();
        client.print(output);
        client.endRequest();

        // Await response
        int statusCode = client.responseStatusCode();
        String response = client.responseBody();

        Serial.print("Status code: ");
        Serial.println(statusCode);
        Serial.print("Response: ");
        Serial.println(response);

        online = (statusCode == 200);
    }
};
WebServer nyxb("nyxb.me");

/*
 * LAN
 */
WiFiServer server(80);
struct Node {
    WiFiClient client;
    IPAddress IP;

    explicit Node(const IPAddress& ip) : IP(ip) {};
}; //Client


/**
 * Clock
 */
struct Clock {
    bool isSet = false;
    const char* ntpServer = "pool.ntp.org";
    const long utcOffsetInSeconds = -25200;

    uRTCLib lib;
    WiFiUDP ntpUDP;
    int tcaPort = 6;

    unsigned long lastRefresh = millis();

    void init() {
        tcaSelect(tcaPort);

        isSet = false;
        lib = uRTCLib();

        for (int attempts = 0; !isSet && attempts < 3; attempts++)
            setTime();
    }

    bool setTime() {
        if (WiFiClass::status() != WL_CONNECTED) {
            Serial.println("WiFi not connected. Cannot sync time.");
            return false;
        }

        configTime(utcOffsetInSeconds, 0, ntpServer);
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo)) {
            Serial.println("Failed to obtain time");
            return false;
        }
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        Serial.println("PRE-YEAR: " + String(timeinfo.tm_year));

        // Set RTC time
        lib.set(timeinfo.tm_sec, timeinfo.tm_min, timeinfo.tm_hour, timeinfo.tm_wday, timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year);

        isSet = true;
        return true;
    }

    String formatted(char type[4] = "none") {
        tcaSelect(tcaPort);

        if (!isSet)
            return "NO CLOCK!";

        if (millis() - lastRefresh > 1000)
            lib.refresh(); // Make sure the data is current

        int year = (lib.year() + 1900) % 100; //Year is years since 1900
        int month = lib.month();
        int day = lib.day();
        int hour = lib.hour();
        int minute = lib.minute();

        String ampm = "am";
        if (hour >= 12) {
            ampm = "pm";
            if (hour > 12)
                hour -= 12;
        }
        if (hour == 0) hour = 12; // Convert 00 to 12 for am

        if (String("date").equals(type)) {
            char dateBuffer[9];
            sprintf(dateBuffer, "%02d/%02d/%02d", month, day, year);
            return {dateBuffer};
        } else if (String("time").equals(type)) {
            char dateBuffer[9];
            sprintf(dateBuffer, "%02d:%02d%s", hour, minute, ampm.c_str());
            return {dateBuffer};
        } else {
            char dateBuffer[20];
            sprintf(dateBuffer, "%02d/%02d/%02d %02d:%02d%s", month, day, year, hour, minute, ampm.c_str());
            return {dateBuffer};
        }
    }

    bool isDay() {
        if (!isSet)
            return false;

        int dayStart = 9;
        int dayEnd = 23;
        return lib.hour() > dayStart && lib.hour() < dayEnd;
    }
};
Clock rtc;


/*
 * Humidifier
 */
struct Humidifier {
    Node node;

    Humidifier()
        : node(IPAddress(192, 168, 0, 232)) {};

    bool isReady = false; //Is the humidifier connected to ShroomNanny?
    bool isOn = false; //Is the humidifier on?

    bool isPerformingAction = false;
    unsigned long performanceStart = millis();

    unsigned long lastPing = 0;
    const unsigned long verificationInterval = 10000; // If no valid ping for 10 seconds then mark as disconnected

    bool onBreak = false;


    //Set target humidity stuff
    int targetInt = 90; //This is the actual var we use to set the target humidity
    bool hitTarget = false;

    int currentTargetIndex = 0; //The index in target[] the user is currently setting
    char target[2] = {'0', '0'};

    int currentPercentage = 0;
    int targetPercentage = 0;

    bool settingTarget = false;

    bool needsRefill = false;

    void toggleSettingTarget() {
        settingTarget = !settingTarget;
    }
    void setByPercentage(int to) {
        if (!isOn || to < 0)
            return;

        targetPercentage = to;

        sendCommand("set_percentage:" + String(to));
    }

    //This is for setting the target on screen with a remote. "Pieces" are each number as it's entered.
    void setTargetPieces(char piece) {
        target[currentTargetIndex] = piece;
        if (currentTargetIndex == 0)
            currentTargetIndex++;
        else {
            String targetString = String(target[0]) + String(target[1]);
            currentTargetIndex = 0;
            setByPercentage((int)targetString.toInt());
        }
    }

    void togglePower() {
        if (isOn)
            sendCommand("turn_off");
        else
            sendCommand("turn_on");
    }

    bool sendCommand(const String& command) {
        if (isPerformingAction)
            return false;

        if (command.equals("reset") || command.equals("turn_on") || command.equals("turn_off") || command.startsWith("set_percentage:"))
            performingAction();

        if (!node.client || !node.client.connected()) {
            Serial.println("Humidifier not connected");
            return false;
        }

        node.client.println(command);
        Serial.println("Command sent to humidifier: " + command);

        return true;
    }

    void performingAction(bool begin = true) {
        if (begin) {
            isPerformingAction = true;
            performanceStart = millis();
        } else
            isPerformingAction = false;
    }

    void killConnection () {
        isReady = false;
        isOn = false;
        node.client.stop(); // Free up resources by stopping the client
    }
};
Humidifier humidifier;


/*
 * Fan
 *
 * HEY NYX!!!!!
 * When you inevitably have to turn the fan up a bit cuz not enough air exchange,
 * do like a couple minutes every half hour or something instead of quick on & offs like this
 */
int fanOnInterval = 150;
int fanOffInterval = 200;
int fanTimer = fanOnInterval;
bool fanOn = true;
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


/*
 * Temp/Humidity Sensors
 */
struct Sensor {
    bool isDHT = false;

    int tca{};

    DHT* dht_sensor = nullptr;
    SHT31* sht_sensor = nullptr;

    float temp = 0;
    float lastTemp = 0;

    float humidity = 0;
    float lastHum = 0;

    // Constructor for DHT sensors
    void init(uint8_t pin, uint8_t type) {
        dht_sensor = new DHT(pin, type);
        isDHT = true;
        dht_sensor->begin();
    }

    // Constructor for SHT sensors
    void init(TwoWire *wire = &Wire) {
        tca = tcaAssigned;
        tcaAssigned++;

        tcaSelect(tca);
        sht_sensor = new SHT31(WIREADDR, wire);
    }

    float readTemperature() const {
        float currTemperature = NAN;
        if (isDHT && dht_sensor != nullptr)
            currTemperature = dht_sensor->readTemperature(true);
        else if (sht_sensor != nullptr) {
            tcaSelect(tca);
            if (sht_sensor->read())
                currTemperature = sht_sensor->getFahrenheit();
        }

        return currTemperature;
    }

    float readHumidity() const {
        float currHumidity = NAN;
        if (isDHT && dht_sensor != nullptr)
            currHumidity = dht_sensor->readHumidity();
        else if (sht_sensor != nullptr) {
            tcaSelect(tca);
            if (sht_sensor->read())
                currHumidity = sht_sensor->getHumidity();
        }

        return currHumidity;
    }

    float maxDiff = 1.0;
    bool changed() {
        float currTemp = readTemperature();
        float currHumidity = readHumidity();

        bool result = false;
        if (!isnan(currTemp) && (isnan(lastTemp) || abs(currTemp - temp) > maxDiff)) {
            Serial.println("Updating " + String(isDHT ? "onboard": "offboard") + " sensor " + String(tca) + " w/temp of " + currTemp);
            lastTemp = temp;
            temp = currTemp;

            result = true;
        }

        if (!isnan(currHumidity) && (isnan(lastHum) || abs(currHumidity - humidity) > maxDiff)) {
            Serial.println("Updating " + String(isDHT ? "onboard": "offboard") + " sensor " + String(tca) + " w/humidity of " + currHumidity);
            lastHum = humidity;
            humidity = currHumidity;

            result = true;
        }

        return result;
    }
};
Sensor onboard_sensor;
Sensor offboard_top_sensor;
Sensor offboard_mid_sensor;
Sensor offboard_bot_sensor;


/**
 * Buzzer
 */
struct Buzzer {
    int pin;

    unsigned long lastWaterTone = 0;
    int waterToneInterval = 30000;

    explicit Buzzer(int buzzerPin) : pin(buzzerPin) {}

    void init() {
        pinMode(pin, OUTPUT);

        //Play a happy little startup arpeggio
        int duration = 200;
        tone(pin, 261, duration);
        tone(pin, 329, duration);
        tone(pin, 392, duration);
        tone(pin, 523, duration);
    }

    void needsWaterTone() {
        if (!rtc.isDay() || millis() - lastWaterTone < waterToneInterval)
            return;

        int duration = 200;
        tone(pin, 523, duration);
        tone(pin, 392, duration);
        tone(pin, 311, duration);
        tone(pin, 392, duration);
        tone(pin, 311, duration);
        tone(pin, 261, duration);

        lastWaterTone = millis();
    }
};
Buzzer buzzer(26);


/**
 * Power Button
 */
struct PowerBtn {
    bool isOn = false;

    PowerBtn() {
        pinMode(36, INPUT);
    }

    bool checkState() {
        isOn = analogRead(36) > 3200;
        return isOn;
    }
};
PowerBtn power;


/*
 * Status Lights
 */
unsigned long lastWaterStatusUpdate;
bool isWaterStatusOff = false;
unsigned long lastPerformingActionUpdate;
bool isPerformingActionOff = false;
void setStatusLights() {
    int blinkLength = 1000;

    //First LED is ShroomNanny status
    if (onboard_sensor.humidity != 0 && onboard_sensor.temp != 0)
        leds[0] = CRGB::Green;
    else
        leds[0] = CRGB::Red;

    //Second LED is WiFi status
    if (WiFiClass::status() != WL_CONNECTED)
        leds[1] = CRGB::Red;
    else
        leds[1] = CRGB::Green;

    //Third light is humidifier status
    if (humidifier.node.client && humidifier.node.client.connected()) {

        if (!humidifier.isOn)
            leds[2] = CRGB::Goldenrod;
        else
            leds[2] = CRGB::Green;

        if (humidifier.needsRefill) {
            buzzer.needsWaterTone();

            if (!isWaterStatusOff)
                leds[2] = CRGB(0, 0, 255);
            else
                leds[2] = CRGB::Black;

            if (currentMillis - lastWaterStatusUpdate > blinkLength) {
                isWaterStatusOff = !isWaterStatusOff;
                lastWaterStatusUpdate = currentMillis;
            }
        } else {
            if (humidifier.isPerformingAction) {
                if (!isPerformingActionOff)
                    leds[2] = CRGB::Green;
                else
                    leds[2] = CRGB::Black;

                if (currentMillis - lastPerformingActionUpdate > blinkLength) {
                    isPerformingActionOff = !isPerformingActionOff;
                    lastPerformingActionUpdate = currentMillis;
                }
            }

            if (humidifier.onBreak)
                leds[2] = CRGB::Amethyst;
        }

    } else
        leds[2] = CRGB::Red;

    FastLED.show();
}


/**
 * Screen
 */
struct Screen {
    TFT_eSPI tft = TFT_eSPI();
    String alert = ""; // If alert ever contains anything we'll display it as a temporary alert.

    void init() {
        tft.init();
        tft.setRotation(1);
        tft.setSwapBytes(true);
        tft.fillScreen(TFT_BLACK);
        tft.drawString("Initializing", 89, 45, 2);
        tft.drawString("ShroomNanny", 62, 65, 2);
        tft.drawString("v" + version, 150, 71, 1);
    }

    void draw(const String& message = "") {

        //First grab our date/time since this can take a second and we don't want to pause mid-render
        String time = rtc.formatted("time");
        String date = rtc.formatted("date");

        //These are our screen dimensions
        int screenW = 240;
        int screenH = 135;

        //Now draw our background
        tft.fillScreen(TFT_BLACK);

        if (!power.isOn) {
            tft.setTextColor(TFT_RED);
            tft.drawString("POWERED OFF", 10, (screenH / 2 - 5), 4);
            return;
        }

        if (alert != "") {
            //Draw IP and version
            tft.setTextColor(TFT_WHITE);

            tft.drawString(alert, 3, 3, 2);
            delay(2000);
            alert = "";
            tft.fillScreen(TFT_BLACK);
        }

        //The image makes text too hard to read atm
//    tft.pushImage(0, 0, 240, 135, Mush);

        //Draw IP and version
        tft.setTextColor(TFT_WHITE);
        tft.drawString("IP: " + WiFi.localIP().toString(), 3, 126, 1);
        tft.drawString("v" + version, 3, 117, 1);

        if (rtc.isSet) {
            tft.drawString(date, 180, 115, 1);
            tft.drawString(time, 182, 126, 1);
        } else {
            tft.setTextColor(TFT_RED);
            tft.drawString("NO TIME!", 170, 126, 1);
        }

        int roomSensorFont = 2;
        int margin = 5;
        tft.setTextColor(TFT_BLUE);
        tft.drawString("Room:", margin, margin, roomSensorFont);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(String(onboard_sensor.temp, 0) + "F - " + String(onboard_sensor.humidity, 0) + "%", 50, margin, roomSensorFont);

        float fanTimerDisplay = (float)fanTimer / 100;
        tft.setTextColor(TFT_GOLD);
        tft.drawString("Fan: ", (screenW - 50), margin, roomSensorFont);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(String(fanTimerDisplay, 0), (screenW - 15), margin, roomSensorFont);



        tft.setTextColor(TFT_GREEN);
        tft.drawString("Tent:", margin, 43, 4);

        int tentReadingsX = 75;
        int tentLineYStart = 28;
        int tentLineYSpacing = 13;
        tft.setTextColor(TFT_WHITE);
        tft.drawString(String(offboard_top_sensor.temp, 0) + "F - " + String(offboard_top_sensor.humidity, 0) + "%", tentReadingsX, tentLineYStart, 2);
        tft.drawString(String(offboard_mid_sensor.temp, 0) + "F - " + String(offboard_mid_sensor.humidity, 0) + "%", tentReadingsX, (tentLineYStart + tentLineYSpacing), 2);
        tft.drawString(String(offboard_bot_sensor.temp, 0) + "F - " + String(offboard_bot_sensor.humidity, 0) + "%", tentReadingsX, (tentLineYStart + tentLineYSpacing * 2), 2);


        int statusY = 87;
        if (humidifier.settingTarget) {
            tft.setTextColor(TFT_RED);
            tft.drawString("Set New Goal: " + String(humidifier.target[0]) + String(humidifier.target[1]), 10, 70, 4);
        } else {
            tft.setTextColor(TFT_CYAN);
            tft.drawString("Target Avg", 18, 90, 1);
            tft.drawString("Humidity", 21, 100, 1);
            tft.drawString(": " + String(humidifier.targetInt) + "%", 80, statusY, 4);

            tft.setTextColor(TFT_WHITE);
            tft.setRotation(tft.getRotation() - 1);
            tft.drawString("Stepper", 45, 160, 1); //X & Y are flipped for this one due to setRotation()
            tft.setRotation(tft.getRotation() + 1);

            String status = String(humidifier.currentPercentage) + "%";
            if (!humidifier.isOn) {
                tft.setTextColor(TFT_RED);
                status = "Off";
            }

            tft.drawString(": " + status, 168, statusY - 25, 4);
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
};
Screen screen;


//Handles IR control signals
void runIrFunc(unsigned long cmd) {
    switch(cmd) {
        case 0xba45ff00: //Power btn
            humidifier.togglePower();
            screen.draw();
            break;
        case 0xb847ff00: //Func btn (set target humidity)
            humidifier.toggleSettingTarget();
            screen.draw();
            break;
        case 0xf807ff00: //Vol down btn
            humidifier.sendCommand("maxout");
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
                screen.alert = "New Target: " + String(humidifier.targetInt) + "%";
                humidifier.settingTarget = false;
                screen.draw();
            } else
                humidifier.setByPercentage(100);
            break;
        case 0xe916ff00: //0
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('0');
                screen.draw();
            } else
                humidifier.setByPercentage(0);
            break;
        case 0xf30cff00: //1
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('1');
                screen.draw();
            } else
                humidifier.setByPercentage(10);
            break;
        case 0xe718ff00: //2
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('2');
                screen.draw();
            } else
                humidifier.setByPercentage(20);
            break;
        case 0xa15eff00: //3
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('3');
                screen.draw();
            } else
                humidifier.setByPercentage(30);
            break;
        case 0xf708ff00: //4
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('4');
                screen.draw();
            } else
                humidifier.setByPercentage(40);
            break;
        case 0xe31cff00: //5
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('5');
                screen.draw();
            } else
                humidifier.setByPercentage(50);
            break;
        case 0xa55aff00: //6
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('6');
                screen.draw();
            } else
                humidifier.setByPercentage(60);
            break;
        case 0xbd42ff00: //7
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('7');
                screen.draw();
            } else
                humidifier.setByPercentage(70);
            break;
        case 0xad52ff00: //8
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('8');
                screen.draw();
            } else
                humidifier.setByPercentage(80);
            break;
        case 0xb54aff00: //9
            if (humidifier.settingTarget) {
                humidifier.setTargetPieces('9');
                screen.draw();
            } else
                humidifier.setByPercentage(90);
            break;
        default:
            if (cmd != 0)
                Serial.println("Uncaught IR command: " + String(cmd, HEX));
            break;
    }
}

/*
 * You know...it's the setup() method
 */
void setup() {
    Serial.begin(115200);
    Serial.println("Initializing ShroomNanny v" + version);

    screen.init();

    currentMillis = millis();
    lastWaterStatusUpdate = currentMillis;


    pinMode(PHOTORESISTOR, INPUT);

    /*
     * Initialize LEDs
     */
    CFastLED::addLeds<WS2812, 32, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(30);
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();

    // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet, IPAddress(8,8,8,8), IPAddress(8,8,4,4)))
        Serial.println("STA Failed to configure");

    /*
     * WiFi
     */
    //Set our status LED
    leds[1] = CRGB::Blue; FastLED.show();
    WiFi.begin(ssid, password);
    while (WiFiClass::status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    leds[1] = CRGB::Green; FastLED.show();

    server.begin(); // Start TCP server


    // Init our DHT11 temp/humidity sensor
    onboard_sensor.init(25, DHT11);

    // Init our SHT30 temp/humidity sensors
    Wire.begin(21, 22); //We need Wire for i2c
    offboard_top_sensor.init(&Wire);
    offboard_mid_sensor.init(&Wire);
    offboard_bot_sensor.init(&Wire);

    rtc.init();

    //Start our fan
    Fan::begin();
    Fan::off();

    //Init the IR receiver
    IrReceiver.begin(38, ENABLE_LED_FEEDBACK);

    buzzer.init();
    screen.draw();
}


/*
 * ...haven't we run this one before?
 */
void loop() {
    bool refreshScreen = false;

    setStatusLights();

    bool lastPowerState = power.isOn;
    if (lastPowerState != power.checkState())
        refreshScreen = true;

    WiFiClient client = server.available(); //Listen for incoming clients
    if (client) {
        IPAddress clientIP = client.remoteIP();
        Serial.print("New Client: ");
        Serial.println(clientIP.toString()); // Print the client's IP address

        if (client.remoteIP() == humidifier.node.IP) {
            humidifier.isReady = false;
            humidifier.node.client = client;
            humidifier.lastPing = millis();
        }
    }

    if (humidifier.node.client && (!humidifier.node.client.connected() || (millis() - humidifier.lastPing > humidifier.verificationInterval))) {
        Serial.println("Humidifier client disconnected.");
        humidifier.killConnection();
    }

    if (humidifier.node.client && humidifier.node.client.connected()) {

        //If a message is available from the humidifier process that first
        if (humidifier.node.client.available()) {
            leds[2] = CRGB::Black;
            FastLED.show();

            humidifier.lastPing = millis(); //Getting a message counts as a ping

            String message = humidifier.node.client.readStringUntil('\n'); // Read data from the client
            message.trim(); // Remove any leading/trailing whitespace and control characters

            //Print all non-redundant messages for our viewing
//            if (!message.equals("ping")) {
//                Serial.println("Received: " + message);
//            }

            if (message.equals("ping")) {
                humidifier.node.client.println("pong");

            } else if (message.equals("ready")) {
                delay(1000);
                humidifier.sendCommand("report_status");

            } else if (message.equals("humidifier_on"))
                humidifier.isOn = true;

            else if (message.equals("humidifier_off"))
                humidifier.isOn = false;

            else if (message.equals("humidifier_reset")) {
                humidifier.isReady = true;
                humidifier.isOn = false;
                humidifier.needsRefill = false;
                humidifier.currentPercentage = 0;

            } else if (message.startsWith("at_percentage:")) {

                //If the humidifier tells us the knob is at a percentage than the following two items HAVE to be true
                humidifier.isReady = true;
                humidifier.isOn = true;

                String percentageStr = message.substring(strlen("at_percentage:"));
                int percentage = percentageStr.toInt();

                Serial.println("Humidifier confirmed position at " + percentageStr + "%.");

                if ((percentage == 0 && humidifier.isOn) || (percentage > 0 && !humidifier.isOn))
                    humidifier.togglePower();

                humidifier.currentPercentage = percentage;

                //If the location we arrived at is not our current target, redirect to our new target
                if (percentage != humidifier.targetPercentage)
                    humidifier.setByPercentage(humidifier.targetPercentage);

            } else if (message.equals("needs_water"))
                humidifier.needsRefill = true;

            if (message.equals("humidifier_reset") || message.equals("humidifier_on") || message.equals("humidifier_off") || message.startsWith("at_percentage:"))
                humidifier.performingAction(false);


        }
    }

    //Automate our humidifer based on existing parameters
    if (!humidifier.settingTarget && humidifier.targetInt > 0 && humidifier.node.client.connected()) {
        Sensor targetSensor = offboard_mid_sensor;
        float avgPercent = (offboard_top_sensor.humidity + offboard_mid_sensor.humidity + offboard_bot_sensor.humidity) / 3;

        int upperBuffer = 5;

        if (humidifier.hitTarget && avgPercent < humidifier.targetInt)
            humidifier.hitTarget = false;

        if (!humidifier.hitTarget && avgPercent >= humidifier.targetInt + upperBuffer)
            humidifier.hitTarget = true;

        if (humidifier.hitTarget && humidifier.isOn) {
            humidifier.togglePower();
        } else {
            // Calculate the inverse percentage of targetSensor.lastHum relative to humidifier.targetInt
            int percentage = 100 - ((avgPercent * 100) / humidifier.targetInt);

            if (humidifier.targetInt - avgPercent > 5)
                percentage += 50;

            // Ensure percentage is within 0 to 100 bounds
            percentage = max(0, min(percentage, 100));


            if (avgPercent >= humidifier.targetInt && targetSensor.lastHum < humidifier.targetInt + upperBuffer)
                percentage = 1;

            if (avgPercent < humidifier.targetInt - 10)
                percentage = 100;
            else
                percentage = 50;

            if (percentage != humidifier.currentPercentage) {
                Serial.println("Setting humidifier to; " + String(percentage));
                if ((humidifier.isOn && percentage <= 0) || (!humidifier.isOn && percentage > 0))
                    humidifier.togglePower();

                humidifier.setByPercentage(percentage);
            }
        }

    }

    //Basic fan logic
    if (fanTimer > 0) {
        fanTimer--;
        if (fanTimer % 100 == 0)
            refreshScreen = true;

    } else {
        if (fanOn) {
            fanTimer = fanOffInterval;
            fanOn = false;
        } else {
            fanTimer = fanOnInterval;
            fanOn = true;
        }
    }

    if (fanOn && humidifier.isOn)
        Fan::on();
    else
        Fan::off();

    //Catch remote commands
    if (IrReceiver.decode()) {
        runIrFunc(IrReceiver.decodedIRData.decodedRawData);
        IrReceiver.resume();
    }

    if (onboard_sensor.changed() || offboard_mid_sensor.changed() || offboard_top_sensor.changed() || offboard_bot_sensor.changed())
        refreshScreen = true;

    if (refreshScreen)
        screen.draw();

    if (humidifier.isPerformingAction && (millis() - humidifier.performanceStart > 1500))
        humidifier.isPerformingAction = false;

    currentMillis = millis();
}