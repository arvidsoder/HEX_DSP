#include "audio_driver.h"

#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>
#include "display_driver.h"
#include "touch_driver.h"
#include "ui.h" // <--- NEW: Include SquareLine Studio header
#include <SerialFlash.h>
// ------------------------------
// LVGL Frame Rate Configuration
// ------------------------------
// 30 FPS = 33 milliseconds per frame
// 20 FPS = 50 milliseconds per frame
// 10 FPS = 100 milliseconds per frame
#define LVGL_FRAME_PERIOD_MS 20 // Targeting approx 30 FPS
unsigned long last_lvgl_tick_ms = 0;
SdFs sd; // SD card object
// GUItool: begin automatically generated code

#define CODEC_RESET_PIN 17
#define BUTTON1_PIN 32
#define BUTTON2_PIN 31
uint8_t effctidx = 0;
uint8_t change_effect = 0;
bool button1;
bool button2;
float distGain;
float distBias;
float distBiasTanh;
float distMaxOutput;

DMAMEM float shape_lut[SHAPE_LEN];

// Load a 16-bit PCM WAV from SD, convert to mono Q15 and install in FIR.
// - path: "/IR.WAV"
// - fir: reference to your AudioFilterFIR instance
// - maxTaps: upper limit (should be <= FIR_MAX_COEFFS)
// - csPin: SD chip select (default BUILTIN_SDCARD for Teensy)
// Returns: number of taps loaded (>0) on success, or negative error code:
//   -1 open failed, -2 bad header, -3 unsupported format, -4 memory allocation failed

void cabIREnable(bool enable) {
  if (enable) {
    cabIR.enable();
    cabMix.gain(0, 0.0);
    cabMix.gain(1, 1.0);
  } else {
    cabIR.disable();
    cabMix.gain(0, 1.0);
    cabMix.gain(1, 0.0);
  }
}

void stringMixersInit(void){
  monoMix1.gain(0, 1.0);
  monoMix1.gain(1, 1.0);
  monoMix1.gain(2, 1.0);
  monoMix1.gain(3, 1.0);
  monoMix2.gain(0, 1.0);
  monoMix2.gain(1, 1.0);
  monoMixOut.gain(0, 1.0);
  monoMixOut.gain(1, 1.0);
  stageMix1.gain(0, 0.0);
  stageMix1.gain(1, 1.0);
  stageMix1.gain(2, 1.0); 
  stageMix1.gain(3, 1.0);
  stageMix2.gain(0, 1.0);
  stageMix2.gain(1, 1.0);
  stageMix2.gain(2, 1.0);
  
  stageMixOut.gain(0, 1.0);
  stageMixOut.gain(1, 1.0);
}



void setup() {
  AudioMemory(200);
  delay(200);
  Serial.begin(115200);
  int taps = load_wav_to_fir("Brohymn Mesa 4x12 SM57 V30 1.wav", cabIR, 200, CAB_IR_TAPS, sd);
  if (taps > 0) { 
  Serial.println("IR ready");
} else {
  Serial.print("Load failed: "); Serial.println(taps);
}
sd.end();

  a_codec.reset(CODEC_RESET_PIN);
  if (!a_codec.enable() || !a_codec.volume(1.0) || !a_codec.inputLevel(1.0))
  {
    Serial.println("Audio Codec CS42448 not found!");
  }
  else
  {
    Serial.println("Audio Codec CS42448 initialized.");
  }
  Serial.println(AUDIO_SAMPLE_RATE_EXACT);
  setStageDistInit(stage1);
  stringMixersInit();
  // ----------------------------------------------------
    // *** CRITICAL TEENSY 4.1 SPI INITIALIZATION ***
    // ----------------------------------------------------
    
    // Explicitly initialize the default SPI bus and set mode/speed for shared bus safety.
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV2); 
    SPI.setDataMode(SPI_MODE0);
    

    pinMode(BUTTON1_PIN, INPUT_PULLUP);
    pinMode(BUTTON2_PIN, INPUT_PULLUP);
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
  if (millis() - last_lvgl_tick_ms >= LVGL_FRAME_PERIOD_MS) {
        lv_tick_inc(millis() - last_lvgl_tick_ms);
        lv_timer_handler();
        last_lvgl_tick_ms = millis();
        button1 = !digitalRead(BUTTON1_PIN);
        button2 = !digitalRead(BUTTON2_PIN);
    }

}
