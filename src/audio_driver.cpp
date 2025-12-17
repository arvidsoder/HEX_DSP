#include <Audio.h>
#include <math.h>
#include "audio_driver.h"



static int resample_linear(float *src, int srcLen, uint32_t srcRate, uint32_t dstRate, 
                           float *out, int maxOut) {
  if (srcRate == dstRate) {
    if (srcLen > maxOut) return -1;
    memcpy(out, src, srcLen * sizeof(float));
    return srcLen;
  }
  
  double ratio = (double)srcRate / dstRate;
  int outLen = (int)(srcLen / ratio + 0.5);
  if (outLen > maxOut) return -1;
  
  for (int i = 0; i < outLen; i++) {
    double frac = i * ratio;
    int idx = (int)frac;
    double f = frac - idx;
    
    if (idx + 1 < srcLen) {
      out[i] = src[idx] * (1.0 - f) + src[idx + 1] * f;
    } else if (idx < srcLen) {
      out[i] = src[idx];
    } else {
      out[i] = 0.0f;
    }
  }
  return outLen;
}

int load_wav_to_fir(const char *path, AudioFilterFIR &fir, int maxDurationMs, int maxTaps, SdFs &sd,uint8_t csPin = BUILTIN_SDCARD) {
  if (!sd.begin(SdioConfig(FIFO_SDIO))) {
    Serial.println("SD.begin failed");
    return -1;
  }
  SdFile f;
  f.open(path, O_READ);
  if (!f) {
    Serial.print("Failed to open: "); Serial.println(path);
    return -1;
  }

  // Parse RIFF/WAVE header
  char riff[4];
  if (f.read((uint8_t*)riff, 4) != 4 || memcmp(riff, "RIFF", 4) != 0) { f.close(); return -2; }
  f.seek(8);
  char wave[4];
  if (f.read((uint8_t*)wave, 4) != 4 || memcmp(wave, "WAVE", 4) != 0) { f.close(); return -2; }

  uint32_t audioFormat=0, numChannels=0, sampleRate=0, bitsPerSample=0;
  uint32_t dataChunkOffset=0, dataChunkSize=0;

  f.seek(12);
  while (f.position() < f.size()) {
    char id[4];
    if (f.read((uint8_t*)id, 4) != 4) break;
    uint32_t chunkSize = 0;
    if (f.read((uint8_t*)&chunkSize, 4) != 4) break;
    
    if (memcmp(id, "fmt ", 4) == 0) {
      uint16_t audioFormat16 = 0, numChannels16 = 0, bitsPerSample16 = 0;
      uint32_t sampleRate32 = 0;
      f.read((uint8_t*)&audioFormat16, 2);
      f.read((uint8_t*)&numChannels16, 2);
      f.read((uint8_t*)&sampleRate32, 4);
      f.seek(f.position() + 6);
      f.read((uint8_t*)&bitsPerSample16, 2);
      audioFormat = audioFormat16;
      numChannels = numChannels16;
      sampleRate = sampleRate32;
      bitsPerSample = bitsPerSample16;
      if (chunkSize > 16) f.seek(f.position() + (chunkSize - 16));
    } else if (memcmp(id, "data", 4) == 0) {
      dataChunkOffset = f.position();
      dataChunkSize = chunkSize;
      f.seek(f.position() + chunkSize);
    } else {
      f.seek(f.position() + chunkSize);
    }
    if (chunkSize & 1) f.seek(f.position() + 1);
  }

  if (dataChunkOffset == 0) { f.close(); return -2; }
  if (audioFormat != 1) { f.close(); return -3; }
  if (bitsPerSample != 16) { f.close(); return -3; }

  uint32_t bytesPerFrame = (bitsPerSample / 8) * numChannels;
  uint32_t framesToRead = (sampleRate * maxDurationMs) / 1000;
  uint32_t framesAvailable = dataChunkSize / bytesPerFrame;
  if (framesToRead > framesAvailable) framesToRead = framesAvailable;
  if (framesToRead > 50000) framesToRead = 50000;

  int rawTaps = (int)framesToRead;
  
  Serial.print("Reading ");
  Serial.print(rawTaps);
  Serial.print(" frames (");
  Serial.print(rawTaps * 1000 / sampleRate);
  Serial.println(" ms) from IR");

  // Allocate only raw float buffer (will be freed after reading)
  float *rawFloat = (float*)malloc(sizeof(float) * rawTaps);
  if (!rawFloat) { f.close(); return -4; }

  f.seek(dataChunkOffset);
  float peak = 0.0f;
  for (int i = 0; i < rawTaps; i++) {
    int32_t acc = 0;
    for (uint16_t c = 0; c < numChannels; c++) {
      int16_t s = 0;
      if (f.read((uint8_t*)&s, 2) != 2) s = 0;
      acc += s;
    }
    float v = (float)acc / (float)(numChannels * 32768.0f);
    rawFloat[i] = v;
    if (fabsf(v) > peak) peak = fabsf(v);
  }

  // Calculate final tap count after resampling
  int finalTaps = rawTaps;
  if (sampleRate != (uint32_t)AUDIO_SAMPLE_RATE_EXACT) {
    Serial.print("Resampling ");
    Serial.print(sampleRate);
    Serial.print(" Hz → ");
    Serial.print((int)AUDIO_SAMPLE_RATE_EXACT);
    Serial.println(" Hz");
    
    finalTaps = (int)(rawTaps * AUDIO_SAMPLE_RATE_EXACT / sampleRate + 0.5);
  }
  if (finalTaps > maxTaps) finalTaps = maxTaps;

  // Normalize peak
  float scale = (peak > 0.0f) ? (0.3f / peak) : 1.0f;
  Serial.print("Peak: "); Serial.print(peak, 4); Serial.print(", Scale: "); Serial.println(scale, 4);

  // Allocate final Q15 coeffs
  int16_t *coeffs = (int16_t*)malloc(sizeof(int16_t) * finalTaps);
  if (!coeffs) { free(rawFloat); f.close(); return -4; }

  // Resample directly into Q15 output (no intermediate buffer)
  if (sampleRate == (uint32_t)AUDIO_SAMPLE_RATE_EXACT) {
    // No resampling needed, just convert
    for (int i = 0; i < finalTaps; i++) {
      float s = rawFloat[i] * scale;
      int32_t q = (int32_t)roundf(s * 32767.0f);
      if (q > 32767) q = 32767;
      if (q < -32768) q = -32768;
      coeffs[i] = (int16_t)q;
    }
  } else {
    // Resample + convert in one pass
    double ratio = (double)sampleRate / AUDIO_SAMPLE_RATE_EXACT;
    for (int i = 0; i < finalTaps; i++) {
      double frac = i * ratio;
      int idx = (int)frac;
      double f = frac - idx;
      
      float v = 0.0f;
      if (idx + 1 < rawTaps) {
        v = rawFloat[idx] * (1.0 - f) + rawFloat[idx + 1] * f;
      } else if (idx < rawTaps) {
        v = rawFloat[idx];
      }
      
      float s = v * scale;
      int32_t q = (int32_t)roundf(s * 32767.0f);
      if (q > 32767) q = 32767;
      if (q < -32768) q = -32768;
      coeffs[i] = (int16_t)q;
    }
  }

  fir.begin((const short*)coeffs, finalTaps);

  free(rawFloat);  // free raw buffer early
  f.close();

  Serial.print("IR ready: ");
  Serial.print(finalTaps);
  Serial.print(" taps @ ");
  Serial.print((int)AUDIO_SAMPLE_RATE_EXACT);
  Serial.println(" Hz");

  return finalTaps;
}


// Constructor - initializes audio objects and connections
EffectMixer8::EffectMixer8(void) 
    : patchInToMix(in, 0, mixerA, 0),
      patchInToRectifier(in, 0, rectifierFx, 0),
      patchRectifierToMix(rectifierFx, 0, mixerA, FX_INDEX_RECTIFIER),
      patchInToEQ(in, 0, eqFx, 0),
      patchEQToMix(eqFx, 0, mixerA, FX_INDEX_EQ),
      patchInToDIST(in, 0, distFx, 0),
      patchDISTToMix(distFx, 0, mixerA, FX_INDEX_DIST),
      patchMixAToOut0(mixerA, 0, out, 0),
      patchMixBToOut1(mixerB, 0, out, 1),
      activeEffect(0),
      wetDry(1.0f)
{
    // Initialize mixer gains
    for (int i = 0; i < 4; i++) {
        mixerA.gain(i, 1.0f);
        mixerB.gain(i, 1.0f);
    }
    out.gain(0, 1.0f);
    out.gain(1, 1.0f);
    in.gain(1.0f);
    
   
   
    
    // Initialize EQ on stage 0
    eqFx.setBandpass(0, 1000.0f, 0.5f);
    
    
    // Disable all e ffects initially
    
    rectifierFx.disable();
    eqFx.disable();
    distFx.disable();
    
}

void EffectMixer8::setInputGain(float level) {
    in.gain(level);
}

void EffectMixer8::gain(uint8_t channel, float level) {
    if (channel < 4)
        mixerA.gain(channel, level);
    else if (channel < 8)
        mixerB.gain(channel - 4, level);
}

void EffectMixer8::muteAllChannels() {
    for (int i = 0; i < 4; i++) {
        mixerA.gain(i, 0.0f);
        mixerB.gain(i, 0.0f);
    }
}

void EffectMixer8::setActiveEffect(uint8_t effectIndex) {
    activeEffect = effectIndex;
    
    // Disable all effects
    for (int i = 1; i < 8; i++) {
        setEffectEnabled(i, false);
    }
    
    // Reset all channel gains
    for (int i = 0; i < 4; i++) {
        mixerA.gain(i, 0.0f);
        mixerB.gain(i, 0.0f);
    }
    
    switch (effectIndex) {
        
        case FX_INDEX_RECTIFIER:
            setEffectEnabled(FX_INDEX_RECTIFIER, true);
            gain(0, 1.0f - wetDry);           // dry input
            gain(FX_INDEX_RECTIFIER, wetDry); // wet rectifier
            break;
        case FX_INDEX_EQ:
            setEffectEnabled(FX_INDEX_EQ, true);
            gain(0, 1.0f - wetDry);           // dry input
            gain(FX_INDEX_EQ, wetDry);        // wet EQ (on mixerB)
            break;
        case FX_INDEX_DIST:
            setEffectEnabled(FX_INDEX_DIST, true);
            gain(0, 1.0f - wetDry);           // dry input
            gain(FX_INDEX_DIST, wetDry);       // wet DIST (on mixerB)
            break;
        default:
            activeEffect = 0;
            gain(0, 1.0f);  // all dry, no effects
            break;
    }
}

void EffectMixer8::setWetDryMix(float32_t wetDryLevel) {
    // Clamp to 0.0 - 1.0
    wetDry = wetDryLevel;
    if (wetDry < 0.0f) wetDry = 0.0f;
    if (wetDry > 1.0f) wetDry = 1.0f;
    
    if (activeEffect == 0) {
        // No active effect, all dry
        gain(0, 1.0f);
    }
    // Set dry path (channel 0 of mixerA)
    gain(0, 1.0f - wetDry);
    // Set wet path (active effect channel)
    gain(activeEffect, wetDry);
}

void EffectMixer8::setEffectEnabled(uint8_t effectIndex, boolean enable) {
    switch (effectIndex) {
        
        case FX_INDEX_RECTIFIER:
            enable ? rectifierFx.enable() : rectifierFx.disable();
            break;
        case FX_INDEX_EQ:
            enable ? eqFx.enable() : eqFx.disable();
            break;
        case FX_INDEX_DIST:
            enable ? distFx.enable() : distFx.disable();
            break;
        default:
            break;
    }
}



void EffectMixer8::setEQ(uint32_t stage, const char *type, float frequency, float qOrGain) {
    if (type == nullptr) return;
    
    if (strcmp(type, "lowpass") == 0) {
        eqFx.setLowpass(stage, frequency, qOrGain);
    } else if (strcmp(type, "highpass") == 0) {
        eqFx.setHighpass(stage, frequency, qOrGain);
    } else if (strcmp(type, "bandpass") == 0) {
        eqFx.setBandpass(stage, frequency, qOrGain);
    } else if (strcmp(type, "notch") == 0) {
        eqFx.setNotch(stage, frequency, qOrGain);
    } else if (strcmp(type, "lowshelf") == 0) {
        eqFx.setLowShelf(stage, frequency, qOrGain);
    } else if (strcmp(type, "highshelf") == 0) {
        eqFx.setHighShelf(stage, frequency, qOrGain);
    }
    
}




