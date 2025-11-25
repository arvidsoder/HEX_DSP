#include <Arduino.h>
#include <lvgl.h>
#include "ui.h"
#include "display_driver.h"

void display_driver_init();
void touch_driver_init();

void setup() {
    Serial.begin(115200);
    lv_init();

    display_driver_init();
    touch_driver_init();
    tft.useFrameBuffer(true);  // Enable frame buffer for better performance
    ui_init();       // SquareLine Studio UI
}

void loop() {
    lv_timer_handler();
    tft.updateScreen();   // REQUIRED if you enabled useFrameBuffer(true)
    delay(5);
}

