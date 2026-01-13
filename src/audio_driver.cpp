#include <Audio.h>
#include <math.h>
#include "audio_driver.h"

// DMAMEM int16_t cabIRCoeffs[CAB_IR_TAPS]; // Removed unused array to save memory
// Access the global shape_lut defined in main.cpp to save stack/RAM1 and ensure persistence
extern float shape_lut[SHAPE_LEN];

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
      patchInToDISTPreGain(in, 0, distPreGain, 0),
      patchInToDIST(distPreGain, 0, distFx, 0),
      patchDISTPostGain(distFx, 0, distPostGain, 0),
      patchDISTToMix(distPostGain, 0, mixerA, FX_INDEX_DIST),
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

void EffectMixer8::setDistParam(uint8_t paramIndex, float paramValue) {
    switch (paramIndex) {
        case 0: // Gain
            
            distPreGain.gain(DIST_INGAIN_DEFAULT * expf(paramValue/10.0f)); // 0 - 100 to exponatial gain
            break;
        case 1: // Bias
            
            break;
        case 2: // MaxOutput
            
            distPostGain.gain(exp10f((paramValue - 100.0f) * 0.01f) * 1.0f);
            break;
    }
}

void EffectMixer8::setEffectParam(uint8_t effectIndex, uint8_t paramIndex, float paramValue) {
  if ((paramIndex == 6)&&(effectIndex != 7)) { // Wet/Dry Mix
            setWetDryMix(paramValue);
            return;
        }    
  switch (effectIndex) {
        case FX_INDEX_RECTIFIER:
            
            break;
        case FX_INDEX_DIST:
            setDistParam(paramIndex, paramValue);

            break;
        default:
            break;
    }
}



void setStageDistInit(EffectMixer8* stage) {
  float distBiasTanh = tanhf(DIST_BIAS_DEFAULT);
  // float shape_lut[SHAPE_LEN]; // REMOVED: Do not shadow the global variable!
  // Use global shape_lut (in DMAMEM) instead of local stack variable
  for (int j = 0; j < SHAPE_LEN; j++) {
      float in = 2.0f * (float)j / (float)(SHAPE_LEN - 1) - 1.0f; // -1.0 to 1.0
      shape_lut[j] = 0.5f*DIST_MAXOUTPUT_DEFAULT * (tanh(DIST_INGAIN_DEFAULT * (in + DIST_BIAS_DEFAULT)) - distBiasTanh);
  }

  for (int i = 0; i < 7; i++) {
  
    stage[i].distFx.shape(shape_lut, SHAPE_LEN); // Default waveshaper amount
    
  }
}

 AudioInputTDM     tdm1;
 AudioOutputTDM    tdm2;
 DMAMEM EffectMixer8      stage1[7];
 DMAMEM EffectMixer8      stage2[7];
 DMAMEM EffectMixer8      stage3[7];
 DMAMEM EffectMixer8      stage4[7];
 DMAMEM EffectMixer8      stage5[7];
 DMAMEM EffectMixer8      stage6[7];
 DMAMEM EffectMixer8      stage7[7];
 AudioAmplifier   masterAmp;

 DMAMEM AudioFilterFIR    cabIR;
 DMAMEM AudioMixer4       cabMix;
 DMAMEM AudioMixer4       monoMix1;
 DMAMEM AudioMixer4       monoMix2;
 DMAMEM AudioMixer4       monoMixOut;
 DMAMEM AudioMixer4       stageMix1;
 DMAMEM AudioMixer4       stageMix2;
 DMAMEM AudioMixer4       stageMixOut;
 DMAMEM AudioControlPCM3168 a_codec;

// ===== Audio connections =====
DMAMEM AudioConnection patchMono0(tdm1, 0, monoMix1, 0);
DMAMEM AudioConnection patchMono1(tdm1, 2, monoMix1, 1);
DMAMEM AudioConnection patchMono2(tdm1, 4, monoMix1, 2);
DMAMEM AudioConnection patchMono3(tdm1, 6, monoMix1, 3);
DMAMEM AudioConnection patchMono4(tdm1, 8, monoMix2, 0);
DMAMEM AudioConnection patchMono5(tdm1, 10, monoMix2, 1);
DMAMEM AudioConnection patchMono6(monoMix1, 0, monoMixOut, 0);
DMAMEM AudioConnection patchMono7(monoMix2, 0, monoMixOut, 1);

DMAMEM AudioConnection patchStage10(monoMixOut, 0, stage1[0].in, 0);
DMAMEM AudioConnection patchStage11(tdm1, 0, stage1[1].in, 0);
DMAMEM AudioConnection patchStage12(tdm1, 2, stage1[2].in, 0);
DMAMEM AudioConnection patchStage13(tdm1, 4, stage1[3].in, 0);
DMAMEM AudioConnection patchStage14(tdm1, 6, stage1[4].in, 0);
DMAMEM AudioConnection patchStage15(tdm1, 8, stage1[5].in, 0);
DMAMEM AudioConnection patchStage16(tdm1, 10, stage1[6].in, 0);

DMAMEM AudioConnection patchstageOut0(stage1[0].out, 0, stage2[0].in, 0);
DMAMEM AudioConnection patchstageOut1(stage1[1].out, 0, stage2[1].in, 0);
DMAMEM AudioConnection patchstageOut2(stage1[2].out, 0, stage2[2].in, 0);
DMAMEM AudioConnection patchstageOut3(stage1[3].out, 0, stage2[3].in, 0);
DMAMEM AudioConnection patchstageOut4(stage1[4].out, 0, stage2[4].in, 0);
DMAMEM AudioConnection patchstageOut5(stage1[5].out, 0, stage2[5].in, 0);
DMAMEM AudioConnection patchstageOut6(stage1[6].out, 0, stage2[6].in, 0);

DMAMEM AudioConnection patchstageOut02(stage2[0].out, 0, stage3[0].in, 0);
DMAMEM AudioConnection patchstageOut12(stage2[1].out, 0, stage3[1].in, 0);
DMAMEM AudioConnection patchstageOut22(stage2[2].out, 0, stage3[2].in, 0);
DMAMEM AudioConnection patchstageOut32(stage2[3].out, 0, stage3[3].in, 0);
DMAMEM AudioConnection patchstageOut42(stage2[4].out, 0, stage3[4].in, 0);
DMAMEM AudioConnection patchstageOut52(stage2[5].out, 0, stage3[5].in, 0);
DMAMEM AudioConnection patchstageOut62(stage2[6].out, 0, stage3[6].in, 0);

DMAMEM AudioConnection patchstageOut03(stage3[0].out, 0, stage4[0].in, 0);
DMAMEM AudioConnection patchstageOut13(stage3[1].out, 0, stage4[1].in, 0);
DMAMEM AudioConnection patchstageOut23(stage3[2].out, 0, stage4[2].in, 0);
DMAMEM AudioConnection patchstageOut33(stage3[3].out, 0, stage4[3].in, 0);
DMAMEM AudioConnection patchstageOut43(stage3[4].out, 0, stage4[4].in, 0);
DMAMEM AudioConnection patchstageOut53(stage3[5].out, 0, stage4[5].in, 0);
DMAMEM AudioConnection patchstageOut63(stage3[6].out, 0, stage4[6].in, 0);

DMAMEM AudioConnection patchstageOut04(stage4[0].out, 0, stage5[0].in, 0);
DMAMEM AudioConnection patchstageOut14(stage4[1].out, 0, stage5[1].in, 0);
DMAMEM AudioConnection patchstageOut24(stage4[2].out, 0, stage5[2].in, 0);
DMAMEM AudioConnection patchstageOut34(stage4[3].out, 0, stage5[3].in, 0);
DMAMEM AudioConnection patchstageOut44(stage4[4].out, 0, stage5[4].in, 0);
DMAMEM AudioConnection patchstageOut54(stage4[5].out, 0, stage5[5].in, 0);
DMAMEM AudioConnection patchstageOut64(stage4[6].out, 0, stage5[6].in, 0);

DMAMEM AudioConnection patchstageOut05(stage5[0].out, 0, stage6[0].in, 0);
DMAMEM AudioConnection patchstageOut15(stage5[1].out, 0, stage6[1].in, 0);
DMAMEM AudioConnection patchstageOut25(stage5[2].out, 0, stage6[2].in, 0);
DMAMEM AudioConnection patchstageOut35(stage5[3].out, 0, stage6[3].in, 0);
DMAMEM AudioConnection patchstageOut45(stage5[4].out, 0, stage6[4].in, 0);
DMAMEM AudioConnection patchstageOut55(stage5[5].out, 0, stage6[5].in, 0);
DMAMEM AudioConnection patchstageOut65(stage5[6].out, 0, stage6[6].in, 0);

DMAMEM AudioConnection patchstageOut06(stage6[0].out, 0, stage7[0].in, 0);
DMAMEM AudioConnection patchstageOut16(stage6[1].out, 0, stage7[1].in, 0);
DMAMEM AudioConnection patchstageOut26(stage6[2].out, 0, stage7[2].in, 0);
DMAMEM AudioConnection patchstageOut36(stage6[3].out, 0, stage7[3].in, 0);
DMAMEM AudioConnection patchstageOut46(stage6[4].out, 0, stage7[4].in, 0);
DMAMEM AudioConnection patchstageOut56(stage6[5].out, 0, stage7[5].in, 0);
DMAMEM AudioConnection patchstageOut66(stage6[6].out, 0, stage7[6].in, 0);


DMAMEM AudioConnection patchstageOut07(stage7[0].out, 0, stageMix1, 0);
DMAMEM AudioConnection patchstageOut17(stage7[1].out, 0, stageMix1, 1);
DMAMEM AudioConnection patchstageOut27(stage7[2].out, 0, stageMix1, 2);
DMAMEM AudioConnection patchstageOut37(stage7[3].out, 0, stageMix1, 3);
DMAMEM AudioConnection patchstageOut47(stage7[4].out, 0, stageMix2, 0);
DMAMEM AudioConnection patchstageOut57(stage7[5].out, 0, stageMix2, 1);
DMAMEM AudioConnection patchstageOut67(stage7[6].out, 0, stageMix2, 2);

DMAMEM AudioConnection patchstageOut7(stageMix1, 0, stageMixOut, 0);
DMAMEM AudioConnection patchstageOut8(stageMix2, 0, stageMixOut, 1);

DMAMEM AudioConnection patchIR(stageMixOut, 0, cabIR, 0);
DMAMEM AudioConnection patchOut(cabIR, 0, cabMix, 1);
DMAMEM AudioConnection patchCabMixLeft(stageMixOut, 0, cabMix, 0);
DMAMEM AudioConnection patchToTDMOut(cabMix, 0, masterAmp, 0);
DMAMEM AudioConnection patchToTDMOut2(masterAmp, 0, tdm2, 0);

void setStageEffect(uint8_t slotIndex, uint8_t effectIndex) {
    switch (slotIndex) {
        case 0:
        for (int i = 0; i < 7; i++) {
            stage1[i].setActiveEffect(effectIndex);
        }
            break;
        case 1:
        for (int i = 0; i < 7; i++) {
            stage2[i].setActiveEffect(effectIndex);
        }
            break;
        case 2:
        for (int i = 0; i < 7; i++) {
            stage3[i].setActiveEffect(effectIndex);
        }
            break;
        case 3:
        for (int i = 0; i < 7; i++) {
            stage4[i].setActiveEffect(effectIndex);
        }
            break;
        case 4:
        for (int i = 0; i < 7; i++) {
            stage5[i].setActiveEffect(effectIndex);
        }
            break;
        case 5:
        for (int i = 0; i < 7; i++) {
            stage6[i].setActiveEffect(effectIndex);
        }
            break;
        case 6:
        for (int i = 0; i < 7; i++) {
            stage7[i].setActiveEffect(effectIndex);
        }
            break;
        default:
            break;
    }
}

void setStageParameter(uint8_t slotIndex, uint8_t effectIndex, uint8_t paramIndex, float paramValue) {
    switch (slotIndex) {
        case 0:
        for (int i = 0; i < 7; i++) {
            stage1[i].setEffectParam(effectIndex, paramIndex, paramValue);
        }
            break;
        case 1:
        for (int i = 0; i < 7; i++) {
            stage2[i].setEffectParam(effectIndex, paramIndex, paramValue);
        }
            break;
        case 2:
        for (int i = 0; i < 7; i++) {
            stage3[i].setEffectParam(effectIndex, paramIndex, paramValue);
        }
            break;
        case 3:
        for (int i = 0; i < 7; i++) {
            stage4[i].setEffectParam(effectIndex, paramIndex, paramValue);
        }
            break;
        case 4:
        for (int i = 0; i < 7; i++) {
            stage5[i].setEffectParam(effectIndex, paramIndex, paramValue);
        }
            break;
        case 5:
        for (int i = 0; i < 7; i++) {
            stage6[i].setEffectParam(effectIndex, paramIndex, paramValue);
        }
            break;
        case 6:
        for (int i = 0; i < 7; i++) {
            stage7[i].setEffectParam(effectIndex, paramIndex, paramValue);
        }
            break;
        default:
            break;
    }
}

void setStageWetDry(uint8_t slotIndex, float wetDryLevel) {
    switch (slotIndex) {
        case 0:
        for (int i = 0; i < 7; i++) {
            stage1[i].setWetDryMix(wetDryLevel);
        }
            break;
        case 1:
        for (int i = 0; i < 7; i++) {
            stage2[i].setWetDryMix(wetDryLevel);
        }
            break;
        case 2:
        for (int i = 0; i < 7; i++) {
            stage3[i].setWetDryMix(wetDryLevel);
        }
            break;
        case 3:
        for (int i = 0; i < 7; i++) {
            stage4[i].setWetDryMix(wetDryLevel);
        }
            break;
        case 4:
        for (int i = 0; i < 7; i++) {
            stage5[i].setWetDryMix(wetDryLevel);
        }
            break;
        case 5:
        for (int i = 0; i < 7; i++) {
            stage6[i].setWetDryMix(wetDryLevel);
        }
            break;
        case 6:
        for (int i = 0; i < 7; i++) {
            stage7[i].setWetDryMix(wetDryLevel);
        }
            break;
        default:
            break;
    }
}

void setMasterOutputLevel(float level) {
    masterAmp.gain(level);
}
