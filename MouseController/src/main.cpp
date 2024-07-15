#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include <cstring>

#include <RH_ASK.h>

#define TX_PIN 15
#define RX_PIN 25
RH_ASK driver(2000, RX_PIN, TX_PIN);

LiquidCrystal_I2C lcd(0x27, 20, 4);

// Draw screen stuff
static char lastLines[4][21] = { "", "", "", "" }; // Static variable to retain its value between function calls
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 250; // in milliseconds

void drawScreen() {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= updateInterval) {
        lastUpdateTime = currentTime;

        lcd.clear();

        for (int i = 0; i < 4; i++) {
            lcd.setCursor(0, i);
            lcd.print(lastLines[i]);
        }
    }
}

float lastAngle = -1;
struct Joystick {
    int vrX;
    int vrY;
    int sw;

    int max = 4095;
    int center[2] = { max / 2, max / 2 };
    bool centered = true;

    int movementBuffer = 50; // How far from the center do we have to move the joystick before it counts
    int angleBuffer = 8;     // How many degrees away from each direction will snap to that direction
    bool lastBtnState = false;

    unsigned long lastMessageTime = 0;
    const unsigned long messageInterval = 500; // in milliseconds

    Joystick(int x, int y, int btn) : vrX(x), vrY(y), sw(btn) {}

    void init() const {
        pinMode(sw, INPUT_PULLUP);
    }

    void read() {
        int x = analogRead(vrX);
        int y = analogRead(vrY);
        bool btnPressed = digitalRead(sw);

        if (btnPressed && !lastBtnState) {
            center[0] = x;
            center[1] = y;
        }

        // Check if the joystick is centered
        centered = (abs(x - center[0]) <= movementBuffer && abs(y - center[1]) <= movementBuffer);

        char lineBuffer1[20];
        snprintf(lineBuffer1, sizeof(lineBuffer1), "X:%d Y:%d Z:%d", x, y, btnPressed);
        strcpy(lastLines[3], lineBuffer1);

        if (!centered) {
            float angle = atan2(center[1] - y, center[0] - x) * 180.0 / PI;

            if (angle < 0)
                angle += 270;
            else {
                angle -= 90;
                if (angle < 0)
                    angle += 360;
            }

            // At this point our angle is correct, now process that
            char angleText[6];
            if (angle > 360 - angleBuffer || angle < 0 + angleBuffer) {
                angle = 0; // Set our direction to a firm up
                strcpy(angleText, "up");
            } else if (angle > 90 - angleBuffer && angle < 90 + angleBuffer) {
                angle = 90; // Set our direction to a firm right
                strcpy(angleText, "right");
            } else if (angle > 180 - angleBuffer && angle < 180 + angleBuffer) {
                angle = 180;
                strcpy(angleText, "down");
            } else if (angle > 270 - angleBuffer && angle < 270 + angleBuffer) {
                angle = 270;
                strcpy(angleText, "left");
            } else {
                snprintf(angleText, sizeof(angleText), "%.2f", angle);
            }

            char message[20];
            if (strcmp(angleText, "up") == 0 || strcmp(angleText, "down") == 0 ||
                strcmp(angleText, "left") == 0 || strcmp(angleText, "right") == 0) {
                strcpy(message, angleText);
            } else {
                snprintf(message, sizeof(message), "%.2f", angle);
            }

            unsigned long currentTime = millis();
            if (currentTime - lastMessageTime >= messageInterval) {
                lastMessageTime = currentTime;

                if (angle != lastAngle) {
                    driver.send((uint8_t *)message, strlen(message));
                    driver.waitPacketSent();
                    Serial.println("Sent: " + String(message));

                    lastAngle = angle;
                }
            }

            Serial.println(angle);
            snprintf(lastLines[0], sizeof(lastLines[0]), "Angle: %s", angleText);
        } else {
            lastLines[0][0] = '\0'; // Clear the line if centered
        }

        lastBtnState = btnPressed;

        drawScreen(); // Call drawScreen() once per loop iteration
    }
};
Joystick stick(33, 32, 4);

void setup() {
    Serial.begin(115200);
    lcd.begin();

    if (!driver.init())
        Serial.println("Rx/Tx init failed");
}

void loop() {
    uint8_t buf[12];
    uint8_t buflen = sizeof(buf);
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
        // Message with a good checksum received, dump it.
        Serial.print("Received: ");
        Serial.println((char*)buf);

        snprintf(lastLines[0], sizeof(lastLines[0]), "Rx: %s", buf);

        const char *msg = "Received message!";
        driver.send((uint8_t *)msg, strlen(msg));
        driver.waitPacketSent();
    }

    stick.read();
}