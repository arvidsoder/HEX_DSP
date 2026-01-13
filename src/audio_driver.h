#pragma once

#ifdef __cplusplus
#include <Audio.h>
#include <SdFat.h>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

// GUItool: end automatically generated code
#define CAB_IR_TAPS 2000
#define SHAPE_LEN 2049
extern int16_t cabIRCoeffs[CAB_IR_TAPS];

#define DIST_INGAIN_DEFAULT    1.0f
#define DIST_BIAS_DEFAULT      0.0f
#define DIST_MAXOUTPUT_DEFAULT 0.1f

// Effect indices for setActiveEffect()

#define FX_INDEX_RECTIFIER   1
#define FX_INDEX_EQ          2
#define FX_INDEX_DIST        3

#ifdef __cplusplus
int resample_linear(float *src, int srcLen, uint32_t srcRate, uint32_t dstRate, 
                           float *out, int maxOut);

int load_wav_to_fir(const char *path, AudioFilterFIR &fir, int maxDurationMs, int maxTaps, SdFs &sd, uint8_t csPin = BUILTIN_SDCARD);

/**how
 * @class EffectMixer8
 * @brief Audio routing and effect management class
 * 
 * Provides 8-channel audio mixing with 5 selectable effects (chorus, reverb, rectifier, EQ, FIR).
 * Supports wet/dry mixing and per-channel gain control.
 */
class EffectMixer8 {
public:
    // Audio objects
    AudioAmplifier in;
    AudioMixer4 mixerA;                 // Input mixer for first 4 channels + effects
    AudioMixer4 mixerB;                 // Secondary mixer for additional effects
    AudioMixer4 out;                    // Output mixer combining mixerA and mixerB
    
    // Effect objects
    
    AudioEffectRectifier rectifierFx;   // Rectifier effect (channel 3)
    AudioFilterBiquad eqFx;             // EQ/Biquad filter (channel 4)

    AudioAmplifier distPreGain;         // Pre-distortion gain control
    AudioEffectWaveshaper distFx;               // FIR filter effect (channel 5)
    AudioAmplifier distPostGain;        // Post-distortion gain control
    
    // Audio connections (patches)
    AudioConnection patchInToMix;
    AudioConnection patchInToRectifier;
    AudioConnection patchRectifierToMix;
    AudioConnection patchInToEQ;
    AudioConnection patchEQToMix;
    AudioConnection patchInToDISTPreGain;
    AudioConnection patchInToDIST;
    AudioConnection patchDISTPostGain;
    AudioConnection patchDISTToMix;
    AudioConnection patchMixAToOut0;
    AudioConnection patchMixBToOut1;


    /**
     * @brief Constructor - initializes mixer gains and default effect settings
     */
    EffectMixer8(void);

    /**
     * @brief Set input gain
     * @param level Gain level (0.0 to 1.0+)
     */
    void setInputGain(float level);

    /**
     * @brief Set channel gain in the mixing stage
     * @param channel Channel index (0-7)
     * @param level Gain level (0.0 to 1.0+)
     */
    void gain(uint8_t channel, float level);

    /**
     * @brief Mute all channels in both mixers
     */
    void muteAllChannels();
    /**
     * @brief Activate a specific effect and disable others
     * @param effectIndex Effect index (CHORUS_FX_INDEX, REVERB_FX_INDEX, etc.)
     */
    void setActiveEffect(uint8_t effectIndex);

    /**
     * @brief Set wet/dry mix level
     * @param wetDry Mix ratio (0.0 = fully dry, 1.0 = fully wet)
     */
    void setWetDryMix(float32_t wetDryLevel);

    float32_t getWetDryMix(void) const { return wetDry; }
    /**
     * @brief Get currently active effect index
     * @return Active effect index
     */
    
    uint8_t getActiveEffect(void) const { return activeEffect; }

    /**
     * @brief Enable/disable a specific effect without changing the active effect
     * @param effectIndex Effect to control
     * @param enable true to enable, false to disable
     */
    void setEffectEnabled(uint8_t effectIndex, boolean enable);



    void setEQ(uint32_t stage, const char *type, float frequency, float qOrGain = 0.7071f);

    /**
     * @brief Get output mixer for patching to other audio objects
     * @return Pointer to output mixer object
     */
    AudioMixer4* getOutputMixer(void) { return &out; }

    void setEffectParam(uint8_t effectIndex, uint8_t paramIndex, float paramValue);

    void setDistParam(uint8_t paramIndex, float paramValue);

    void setDistGain(float inGain);

    void setDistBias(float Bias);

    void setDistLevel(float outLevel);

private:
    uint8_t activeEffect;  // Currently active effect index
    float32_t wetDry;      // Wet/dry mix ratio
    
    
    
    // Current number of chorus voices (kept so we can re-init chorus when changing delay)


    
};

void setStageDistInit(EffectMixer8* stage) ;

extern AudioInputTDM     tdm1;
extern AudioOutputTDM    tdm2;
extern EffectMixer8      stage1[7];
extern EffectMixer8      stage2[7];
extern EffectMixer8      stage3[7];
extern EffectMixer8      stage4[7];
extern EffectMixer8      stage5[7];
extern EffectMixer8      stage6[7];
extern EffectMixer8      stage7[7];
extern AudioFilterFIR    cabIR;
extern AudioMixer4       cabMix;
extern AudioMixer4       monoMix1;
extern AudioMixer4       monoMix2;
extern AudioMixer4       monoMixOut;
extern AudioMixer4       stageMix1;
extern AudioMixer4       stageMix2;
extern AudioMixer4       stageMixOut;
extern AudioControlPCM3168 a_codec;

// ===== Audio connections =====
extern AudioConnection patchMono0;
extern AudioConnection patchMono1;
extern AudioConnection patchMono2;
extern AudioConnection patchMono3;
extern AudioConnection patchMono4;
extern AudioConnection patchMono5;
extern AudioConnection patchMono6;
extern AudioConnection patchMono7;

extern AudioConnection patchStage10;
extern AudioConnection patchStage11;
extern AudioConnection patchStage12;
extern AudioConnection patchStage13;
extern AudioConnection patchStage14;
extern AudioConnection patchStage15;
extern AudioConnection patchStage16;

extern AudioConnection patchstageOut0;
extern AudioConnection patchstageOut1;
extern AudioConnection patchstageOut2;
extern AudioConnection patchstageOut3;
extern AudioConnection patchstageOut4;
extern AudioConnection patchstageOut5;
extern AudioConnection patchstageOut6;

extern AudioConnection patchstageOut02;    
extern AudioConnection patchstageOut12;
extern AudioConnection patchstageOut22;
extern AudioConnection patchstageOut32;
extern AudioConnection patchstageOut42;
extern AudioConnection patchstageOut52;
extern AudioConnection patchstageOut62;

extern AudioConnection patchstageOut03;
extern AudioConnection patchstageOut13;
extern AudioConnection patchstageOut23;
extern AudioConnection patchstageOut33;
extern AudioConnection patchstageOut43;
extern AudioConnection patchstageOut53;
extern AudioConnection patchstageOut63;

extern AudioConnection patchstageOut04;
extern AudioConnection patchstageOut14;
extern AudioConnection patchstageOut24;
extern AudioConnection patchstageOut34;
extern AudioConnection patchstageOut44;
extern AudioConnection patchstageOut54;
extern AudioConnection patchstageOut64;


extern AudioConnection patchstageOut05; 
extern AudioConnection patchstageOut15;
extern AudioConnection patchstageOut25;
extern AudioConnection patchstageOut35;
extern AudioConnection patchstageOut45;
extern AudioConnection patchstageOut55;
extern AudioConnection patchstageOut65;

extern AudioConnection patchstageOut06; 
extern AudioConnection patchstageOut16;
extern AudioConnection patchstageOut26;
extern AudioConnection patchstageOut36;
extern AudioConnection patchstageOut46;
extern AudioConnection patchstageOut56;
extern AudioConnection patchstageOut66;

extern AudioConnection patchstageOut07; 
extern AudioConnection patchstageOut17;
extern AudioConnection patchstageOut27;
extern AudioConnection patchstageOut37;
extern AudioConnection patchstageOut47;
extern AudioConnection patchstageOut57;
extern AudioConnection patchstageOut67;



extern AudioConnection patchstageOut7;
extern AudioConnection patchstageOut8;

extern AudioConnection patchIR;
extern AudioConnection patchOut;
extern AudioConnection patchOuttstr;
extern AudioConnection patchCabMixLeft;
extern AudioConnection patchToTDMOut;
#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

void setStageEffect(uint8_t slotIndex, uint8_t effectIndex);

void setStageParameter(uint8_t slotIndex, uint8_t effectIndex, uint8_t paramIndex, float paramValue);

void setStageWetDry(uint8_t slotIndex, float wetDryLevel);

void setMasterOutputLevel(float level);


#ifdef __cplusplus
}
#endif