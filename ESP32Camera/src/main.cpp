#include <Arduino.h>
#include "esp_camera.h"
#include "SPI.h"
#include "FS.h"
#include "SPIFFS.h"


//this code will use an ov7670 camera and an esp32 dev kit
#define PWDN -1
#define RST -1
#define D0 36
#define D1 39
#define D2 34
#define D3 35
#define D4 32
#define D5 33
#define D6 25
#define D7 26
#define MCLK 5
#define PCLK 14
#define VSYNC 27
#define HS 23
#define SDA 21
#define SCL 22

void setup() {
    Serial.begin(115200);

    // Check if PSRAM is available
    if (psramFound()) {
        Serial.println("PSRAM is found and available.");
    } else {
        Serial.println("PSRAM is not available.");
    }

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = D0;
    config.pin_d1 = D1;
    config.pin_d2 = D2;
    config.pin_d3 = D3;
    config.pin_d4 = D4;
    config.pin_d5 = D5;
    config.pin_d6 = D6;
    config.pin_d7 = D7;
    config.pin_xclk = MCLK;
    config.pin_pclk = PCLK;
    config.pin_vsync = VSYNC;
    config.pin_href = HS;
    config.pin_sccb_sda = SDA;
    config.pin_sccb_scl = SCL;
    config.pin_pwdn = PWDN;
    config.pin_reset = RST;
    config.xclk_freq_hz = 10000000; // OV7670 can operate at 10MHz XCLK
    config.pixel_format = PIXFORMAT_GRAYSCALE;

    // Set frame size to CIF which is smaller than QVGA
    config.frame_size = FRAMESIZE_96X96; // Adjust based on your requirements
    config.jpeg_quality = 12; // Quality setting is irrelevant for non-JPEG
    config.fb_count = 1; // Only use one frame buffer

    // Camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
}

void loop() {
    // Take Picture with Camera
    camera_fb_t * fb = esp_camera_fb_get();
    if(!fb) {
        Serial.println("Camera capture failed");
        delay(1000);
        return;
    }

    // Print on Serial the Image
    Serial.write(fb->buf, fb->len);
    // Return the frame buffer back to the driver for reuse
    esp_camera_fb_return(fb);
    delay(5000); // Delay for demonstration purpose
}