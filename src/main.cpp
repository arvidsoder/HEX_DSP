#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputTDM            tdm1;           
AudioOutputTDM           tdm2;          
AudioEffectFreeverb     freeverb1;
AudioEffectFreeverb     freeverb2;
      
AudioEffectChorus      chorusFx;
AudioEffectDelay         delayFx;
AudioMixer4              mixer1;
AudioMixer4              mixer2;
AudioMixer4              mixer3;



AudioSynthWaveform       sine1;
AudioSynthWaveform       sine2;
AudioSynthWaveform       sine3;
AudioSynthWaveform       sine4;

AudioConnection          patchCord0(tdm1, 0, mixer1, 0);
AudioConnection          patchCord1(tdm1, 0, freeverb1, 0);
AudioConnection          patchCord2(freeverb1, 0, mixer1, 1);
AudioConnection          patchCord3(mixer1, 0, tdm2, 0);

AudioConnection          patchCord01(tdm1, 2, mixer2, 0);
AudioConnection          patchCord11(tdm1, 2, freeverb2, 0);
AudioConnection          patchCord21(freeverb2, 0, mixer2, 1);
AudioConnection          patchCord31(mixer2, 0, tdm2, 2);


//AudioConnection          patchCord00(freeverb1, 0, tdm2, 0);
//AudioConnection          patchCord01(tdm1, 2, freeverb1, 1);
AudioConnection          patchCord02(tdm1, 2, tdm2, 2);
AudioConnection          patchCord03(tdm1, 4, tdm2, 4);
AudioConnection          patchCord04(tdm1, 6, tdm2, 6);
AudioConnection          patchCord05(tdm1, 8, tdm2, 8);
AudioConnection          patchCord06(tdm1, 10, tdm2, 10);
AudioConnection          patchCord07(tdm1, 12, tdm2, 12);



AudioControlCS42448      cs42448_1;      //xy=273,405
// GUItool: end automatically generated code


void setup() {
  AudioMemory(1000);
  delay(1000);
  Serial.begin(115200);

  if (!cs42448_1.enable() || !cs42448_1.volume(0.7))
  {
    Serial.println("Audio Codec CS42448 not found!");
  }
  else
  {
    Serial.println("Audio Codec CS42448 initialized.");
  }
  Serial.println(AUDIO_SAMPLE_RATE_EXACT);
  freeverb1.roomsize(0.7);
  freeverb1.damping(0.01);
  freeverb2.roomsize(0.7);
  freeverb2.damping(0.01);
  
  sine1.frequency(40);
  sine1.amplitude(0.7);
  sine2.frequency(550);
  sine2.amplitude(0.1);
  sine3.frequency(660);
  sine3.amplitude(0.1);
  sine4.frequency(770);
  sine4.amplitude(0.1);
  
}

void loop() {
  if (millis() % 5000 < 10)
  {
    freeverb1.enable();
    mixer1.gain(1, 1.0f);
   mixer1.gain(0, 0.0f);
   freeverb2.enable();
    mixer2.gain(1, 1.0f);
   mixer2.gain(0, 0.0f);
     
  }
  else if (millis() % 5000 > 2500 && millis() % 5000 < 2600)
  {
    freeverb1.disable();
    mixer1.gain(0, 1.0f);
   mixer1.gain(1, 0.0f);
    freeverb2.disable();
    mixer2.gain(0, 1.0f);
   mixer2.gain(1, 0.0f);
  }
}