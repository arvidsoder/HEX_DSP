#include <Audio.h>
#include <Wire.h>
#include "control_pcm3168.h"
#define PCM3168_RESET_PIN 17 


// PCM3168 I2C control
AudioControlPCM3168 pcm3168;

// TDM (SAI1)
AudioInputTDM    tdm1_in;
AudioOutputTDM   tdm1_out;

// Pitch detector
AudioAnalyzeNoteFrequency  notefreq;

// Route TDM input chan 0 → pitch detector
AudioConnection patchCord0(tdm1_in, 0, notefreq, 0);

// (Optional) echo input to output
AudioConnection patchCord1(tdm1_in, 0, tdm1_out, 0);

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=== PCM3168 + SAI1 + Pitch Detection Test ===");
  
  AudioMemory(100); // plenty, for queues and TDM

  pinMode(LED_BUILTIN,OUTPUT);
  pcm3168.reset(PCM3168_RESET_PIN);
  pcm3168.enable();
  Wire.begin();          // I2C0 on Teensy pins 18/19
  Wire.setClock(400000);
  

  if (!pcm3168.enable()) {
    Serial.println("❌ PCM3168 init failed");
  } else {
    Serial.println("✅ PCM3168 enabled");
  }

  pcm3168.volume(0.8);

  notefreq.begin(0.3f);   // sensitivity (0.0–1.0), default 0.15
}

void loop() {
  if (notefreq.available()) {
    float freq = notefreq.read();
    float prob = notefreq.probability();

    Serial.print("Freq: ");
    Serial.print(freq);
    Serial.print(" Hz   | Confidence: ");
    Serial.println(prob, 2);
  }
}
