#include "audio_driver.h"
#include <SdFat.h>
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
AudioInputTDM            tdm1;           
AudioOutputTDM           tdm2;          
EffectMixer8           stage1;             //xy=120,256
AudioFilterFIR          cabIR;              //xy=330,150
AudioMixer4              cabMix;             //xy=480,150

AudioConnection        patchStage1(tdm1, 0, stage1.in, 0); //xy=198,303
AudioConnection        patchIR(stage1.out, 0, cabIR, 0); //xy=198,303
AudioConnection        patchOut(cabIR, 0, cabMix, 1); //xy=198,303
AudioConnection        patchCabMixLeft(stage1.out, 0, cabMix, 0); //xy=198,303
AudioConnection        patchToTDMOut(cabMix, 0, tdm2, 0); //xy=198,303
AudioControlCS42448      cs42448_1;      //xy=273,405

// GUItool: end automatically generated code
const int16_t cabIRTaps = 2000;
int16_t cabIRCoeffs[cabIRTaps];
uint8_t effctidx = 0;
uint8_t change_effect = 0;

#define BUTTON1_PIN 32
#define BUTTON2_PIN 31
bool button1;
bool button2;

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

void setup() {
  AudioMemory(1000);
  delay(1000);
  Serial.begin(115200);
  int taps = load_wav_to_fir("Brohymn Mesa 4x12 SM57 V30 1.wav", cabIR, 200, cabIRTaps, sd);
  if (taps > 0) { 
  Serial.println("IR ready");
} else {
  Serial.print("Load failed: "); Serial.println(taps);
}
sd.end();
  if (!cs42448_1.enable() || !cs42448_1.volume(1.0) || !cs42448_1.inputLevel(1.0))
  {
    Serial.println("Audio Codec CS42448 not found!");
  }
  else
  {
    Serial.println("Audio Codec CS42448 initialized.");
  }
  Serial.println(AUDIO_SAMPLE_RATE_EXACT);

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

  if (millis() % 1000 == 0 && change_effect == 1)
  {
    change_effect = 0;
    if (button1)
    {
      effctidx ++;
    }
    
    
    if (button2)
    {
      cabIREnable(!cabIR.isEnabled());
    }
    
    stage1.setActiveEffect(effctidx % 6);
    Serial.print("Active Effect Index: ");
    Serial.print(stage1.getActiveEffect());
    Serial.print("   Cab IR: ");
    Serial.println(cabIR.isEnabled() ? "Enabled" : "Disabled");
    
    stage1.setWetDryMix(0.9f);
  }
  else if (millis() % 1000 > 500 && millis() % 1000 < 510)
  {
    change_effect = 1;


  }
}

