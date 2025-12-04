#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <lvgl.h>
#include "display_driver.h"
#include "touch_driver.h"
#include "ui.h" // <--- NEW: Include SquareLine Studio header
#include <SdFat.h>
#include <Audio.h>

// ------------------------------
// LVGL Frame Rate Configuration
// ------------------------------
// 30 FPS = 33 milliseconds per frame
// 20 FPS = 50 milliseconds per frame
// 10 FPS = 100 milliseconds per frame
#define LVGL_FRAME_PERIOD_MS 20 // Targeting approx 30 FPS
// ------------------------------
SdFs sd; // SD card object
// Global variable for simple debug timing

unsigned long last_lvgl_tick_ms = 0;

void setup() {
    Serial.begin(115200);
    // Wait for USB Serial for up to 5s
    while (!Serial && (millis() < 5000)) ; 
    
    Serial.println("--- Teensy 4.1 LVGL DSP Init ---");
    if (sd.begin(SdioConfig(FIFO_SDIO))) {
        Serial.println("SD Card initialized successfully.");
    } else {
        Serial.println("SD Card initialization failed!");
    }
    
    sd.ls("/", LS_R | LS_SIZE);
    SdFile file;
    const char* filename = "Brohymn Mesa 4x12 SM57 V30 1.wav";

    if (!file.open(filename, O_READ)) {
        Serial.print("ERROR: Failed to open IR file: ");
        Serial.println(filename);
    }
    // Seek to the Sample Rate location (byte 24)
    uint32_t sample_rate = 0;
    uint32_t file_size = 0;
    uint32_t resolution = 0;

    if (file.seek(24)) {
        // Read 4 bytes (uint32_t) into the sample_rate variable
        if (file.read(&sample_rate, 4) == 4) {
             Serial.print("WAV Header Sample Rate (from file): ");
             Serial.print(sample_rate);
             Serial.println(" Hz.");
             
             if (sample_rate != 48000) {
                 Serial.println("WARNING: File sample rate does not match DSP rate (48000 Hz).");
             }
        }
    }
    if (file.seek(40)) {
        // Read 4 bytes (uint32_t) into the file_size variable
        if (file.read(&file_size, 4) == 4) {
             Serial.print("WAV Header Data Size (from file): ");
             Serial.print(file_size);
             Serial.println(" bytes.");
        }
    }
    if (file.seek(34)) {
        // Read 2 bytes (uint16_t) into the resolution variable
        if (file.read(&resolution, 2) == 2) {
             Serial.print("WAV Header Bit Depth (from file): ");
             Serial.print(resolution);
             Serial.println(" bits.");
        }
    }
    
    for (uint32_t i = 44; i < file_size + 44; i += resolution / 4) {
        int16_t sample = 0;
        if (file.seek(i)) {
            if (file.read(&sample, 2) == 2) {
                Serial.print("Sample ");
                Serial.print(i-44);
                Serial.print(": ");
                Serial.println(sample);
            }
        }
    }
    Serial.println(resolution);
    Serial.println(file_size);
    Serial.println(sample_rate);
    Serial.println("Starting LVGL 8.3 Project Initialization...");

    // ----------------------------------------------------
    // *** CRITICAL TEENSY 4.1 SPI INITIALIZATION ***
    // ----------------------------------------------------
    
    // Explicitly initialize the default SPI bus and set mode/speed for shared bus safety.
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2); 
    SPI.setDataMode(SPI_MODE0);
    
    // Force Touch CS HIGH to ensure the touch controller is inactive.
    pinMode(T_CS, OUTPUT); 
    digitalWrite(T_CS, HIGH);
    
    // ----------------------------------------------------

    // 1. Initialize the LVGL core
    lv_init();
    
    // 2. Initialize the display driver (TFT_CS, TFT_DC, TFT_RST, flush callback)
    display_driver_init(); 

    // 3. Initialize the touchscreen driver (Touch_CS, Touch_IRQ, read callback)
    touch_driver_init(); 

    // 4. Create your actual UI
    ui_init(); // <--- NEW: Call the SquareLine Studio initialization function
    

    Serial.println("LVGL UI Initialized and Running!");
}

void loop() {
    // LVGL MUST be called frequently to handle drawing, events, and timers.
    
    // ** FIX: Manually increment LVGL's internal time clock **
    if (millis() - last_lvgl_tick_ms >= LVGL_FRAME_PERIOD_MS) {
        lv_tick_inc(millis() - last_lvgl_tick_ms);
        lv_timer_handler();
        last_lvgl_tick_ms = millis();
    }
    //Serial.println(lv_arc_get_value(ui_Arc8));
   
    
}