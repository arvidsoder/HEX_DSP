#include <Audio.h>
#include <Wire.h>
#include "control_pcm3168.h"
#define PCM3168_RESET_PIN 17
// PCM3168 control object (I2C)
AudioControlPCM3168 pcm3168;

// TDM2 audio streams (SAI2)
AudioInputTDM2    tdm2_in;
AudioOutputTDM2   tdm2_out;

// Simple pass-through for all channels
AudioConnection patchCord0(tdm2_in, 0, tdm2_out, 0);
AudioConnection patchCord1(tdm2_in, 1, tdm2_out, 1);
AudioConnection patchCord2(tdm2_in, 2, tdm2_out, 2);
AudioConnection patchCord3(tdm2_in, 3, tdm2_out, 3);
AudioConnection patchCord4(tdm2_in, 4, tdm2_out, 4);
AudioConnection patchCord5(tdm2_in, 5, tdm2_out, 5);


void setup() {
 

  AudioMemory(100); // plenty, for queues and TDM

  pinMode(LED_BUILTIN,OUTPUT);
  pcm3168.reset(PCM3168_RESET_PIN);
 
  while(!Serial)
    ;

  if (CrashReport)
    Serial.print(CrashReport);   
    

  // Initialize PCM3168A

  pcm3168.enable();
  pcm3168.volume(0.7);
  pcm3168.inputLevel(0, 10.0f);
  Serial.begin(115200);
  Serial.println("PCM3168A via SAI2 TDM started");
}

void loop() {
  // Optional: adjust levels or mute dynamically

}
