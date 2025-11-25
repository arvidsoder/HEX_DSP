#include <Arduino.h>
#include <Wire.h>
#include <Audio.h>
#include "control_pcm3168.h"

AudioControlPCM3168      pcm3168;
#define PCM3168_RESET_PIN 17   // Teensy pin wired to PCM3168A RST (pin 4)

// Audio objects just to turn on SAI1 clocks + test tone
AudioSynthWaveformSine   sine1;
AudioInputTDM            tdm1_in;
AudioOutputTDM           tdm1_out;
AudioConnection          patchCord0(sine1, 0, tdm1_out, 0);

void probe_i2c_pcm3168() {
  delay(100); // wait for any prior activity to settle
  uint8_t possible[] = {0x44, 0x45, 0x46, 0x47};

  Serial.println("=== Focused I2C probe for PCM3168A on Wire (SDA=18, SCL=19) ===");

  for (uint8_t i = 0; i < 4; i++) {
    uint8_t addr = possible[i];
    Serial.print("Probing address 0x");
    Serial.print(addr, HEX);
    Serial.print(" ... ");

    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.println("FOUND (ACK)");
    } else if (error == 2) {
      Serial.println("NACK (no device / not in I2C mode)");
    } else if (error == 4) {
      Serial.println("OTHER ERROR (bus error / line held low)");
    } else {
      Serial.print("Error code: ");
      Serial.println(error);
    }
  }

  Serial.println("I2C probe complete.\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== PCM3168A CLOCK + RESET + I2C TEST ===");

  // 1) Start audio system to turn on SAI1 clocks (MCLK/BCLK/LRCLK)
  AudioMemory(20);

  // 440 Hz test tone so we also get audio activity
  sine1.frequency(440.0f);
  sine1.amplitude(0.5f);

  // 2) Manually control RESET so MODE is sampled *with* SCKI running
  pcm3168.reset(PCM3168_RESET_PIN);                           // allow internal reset to finish

  // 3) Bring up I2C on Wire (pins 18/19)
  Wire.begin();
  Wire.setClock(400000);

  // 4) Probe I2C addresses 0x44–0x47
  probe_i2c_pcm3168();

  Serial.println("If FOUND (ACK) appeared above, I2C is now working.");
  Serial.println("You should also hear/see 440 Hz on VOUT1+/−.");
  pcm3168.enable();
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > 2000) {
    last = millis();
    Serial.println("Running (audio + I2C test)...");
  }
}