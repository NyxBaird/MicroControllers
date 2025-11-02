#include <Arduino.h>

struct Button {
    int button;
    int light;

    bool pressed = false;
    bool lightOn = false;

    Button (int button, int light) : button(button), light(light) {}

    void init() const {
        pinMode(button, INPUT_PULLUP);
        pinMode(light, OUTPUT);
    }

    bool isPressed() {
        const bool state = digitalRead(button) == LOW;
        if (pressed == state)
            return state;

        pressed = state;

        Serial.println("btn " + String(state ? "pressed" : "released"));

        return state;
    }

    void turnLightOn() {
        Serial.println("turn light on: " + String(light) + " | " + String(lightOn));

        if (lightOn)
            return;

        Serial.println("light on");

        digitalWrite(light, HIGH);
        lightOn = true;
    }

    void turnLightOff() {
        Serial.println("turn light off: " + String(light) + " | " + String(lightOn));

        if (!lightOn)
            return;

        Serial.println("light off");

        digitalWrite(light, LOW);
        lightOn = false;
    }

    void lightWhenPressed() {
        if (isPressed() && !lightOn)
            turnLightOn();
        else if (!isPressed() && lightOn)
            turnLightOff();
    }
};
Button
    Play(4, 14),
    Stop(3, 15),
    Record(2, 16),
    Overdub(9, 17),
    Automation(8, 18),
    ReAuto(7, 19),
    Capture(6, 20),
    SessionRecord(5, 21);

void setup() {
    Serial.begin(115200);
    Serial.println("Hello World");

    Play.init();
    Stop.init();
    Record.init();
    Overdub.init();
    Automation.init();
    ReAuto.init();
    Capture.init();
    SessionRecord.init();
}

void loop() {
    Play.lightWhenPressed();
    Stop.lightWhenPressed();
    Record.lightWhenPressed();
    Overdub.lightWhenPressed();
    Automation.lightWhenPressed();
    ReAuto.lightWhenPressed();
    Capture.lightWhenPressed();
    SessionRecord.lightWhenPressed();
}