#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#include "display_driver.h"
#include "touch_driver.h"

// ------------------------------
// LVGL Test UI Function Declaration
// ------------------------------
void lv_demo_ui();

// Simple event handler for the button
static void btn_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    if (code == LV_EVENT_CLICKED) {
        Serial.println("BUTTON CLICKED - Touch Input Confirmed!");
        lv_label_set_text(label, "Clicked!");
    }
}

// Global variable for simple debug timing
unsigned long last_debug_print = 0;
unsigned long last_heartbeat = 0;

void setup() {
    Serial.begin(115200);
    // Wait for USB Serial for up to 5s
    while (!Serial && (millis() < 5000)) ; 
    
    Serial.println("--- Teensy 4.1 LVGL DSP Init ---"); // Added separator
    Serial.println("Starting LVGL 8.3 Project Initialization...");

    // ----------------------------------------------------
    // *** CRITICAL TEENSY 4.1 SPI INITIALIZATION ***
    // ----------------------------------------------------
    
    // Explicitly initialize the default SPI bus and set mode/speed for shared bus safety.
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2); // Example slow clock for initial SPI setup (e.g., 30MHz)
    SPI.setDataMode(SPI_MODE0);
    
    // Force Touch CS HIGH to ensure the touch controller is inactive.
    pinMode(T_CS, OUTPUT); 
    digitalWrite(T_CS, HIGH);
    
    // ----------------------------------------------------

    // 1. Initialize the LVGL core
    lv_init();
    
    // ** REMOVED LV_TICK_SET_CB **: We now drive the tick directly in the loop.

    // 2. Initialize the display driver (TFT_CS, TFT_DC, TFT_RST, flush callback)
    display_driver_init(); 

    // 3. Initialize the touchscreen driver (Touch_CS, Touch_IRQ, read callback)
    touch_driver_init(); 

    // 4. Create your actual UI
    lv_demo_ui();

    Serial.println("LVGL UI Initialized and Running!");
}

void loop() {
    // LVGL MUST be called frequently to handle drawing, events, and timers.
    
    // ** FIX: Manually increment LVGL's internal time clock **
    // This is necessary to trigger LVGL's input/timer scheduler.
    lv_tick_inc(5); 

    lv_timer_handler();
    delay(5); 

    // --- MAIN LOOP HEARTBEAT ---
    if (millis() - last_heartbeat > 1000) {
        Serial.println("Loop Heartbeat: OK");
        last_heartbeat = millis();
    }
    
    // --- TOUCH DEBUGGING IN MAIN LOOP (V8.3.6 Compliant & Stable) ---
    // If the touch is active (meaning the read callback ran and detected a press)
    if (is_touch_active()) {
        
        lv_point_t p;
        lv_indev_get_point(touch_indev, &p);

        // We only print if the point is within the screen bounds to filter noise
        if (p.x >= 0 && p.x < 320 && p.y >= 0 && p.y < 240) {
            Serial.print("MAIN Loop Touch Polled at MAPPED (X,Y): (");
            Serial.print(p.x);
            Serial.print(", ");
            Serial.print(p.y);
            Serial.println(")");
        }
    }
}

// ------------------------------
// Simple Test UI (Demonstrates basic functionality)
// ------------------------------
void lv_demo_ui() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_scr_load(scr);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Teensy 4.1 Audio DSP\nLVGL UI OK!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *btn = lv_btn_create(scr);
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    
    // Add the event handler to confirm touch is working in LVGL
    lv_obj_add_event_cb(btn, btn_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me");
    lv_obj_center(btn_label);
}