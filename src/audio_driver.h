#pragma once

#include <Audio.h>


// Effect indices for setActiveEffect()
#define FX_INDEX_CHORUS      1
#define FX_INDEX_REVERB      2
#define FX_INDEX_RECTIFIER   3
#define FX_INDEX_EQ          4
#define FX_INDEX_DIST         5


static int resample_linear(float *src, int srcLen, uint32_t srcRate, uint32_t dstRate, 
                           float *out, int maxOut);

int load_wav_to_fir(const char *path, AudioFilterFIR &fir, int maxDurationMs, int maxTaps, SdFs &sd, uint8_t csPin = BUILTIN_SDCARD);

/**
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
    AudioEffectChorus chorusFx;         // Chorus effect (channel 1)
    AudioEffectFreeverb reverbFx;       // Reverb effect (channel 2)
    AudioEffectRectifier rectifierFx;   // Rectifier effect (channel 3)
    AudioFilterBiquad eqFx;             // EQ/Biquad filter (channel 4)
    AudioEffectWaveshaper distFx;               // FIR filter effect (channel 5)
    
    // Audio connections (patches)
    AudioConnection patchInToMix;
    AudioConnection patchInToChorus;
    AudioConnection patchChorusToMix;
    AudioConnection patchInToReverb;
    AudioConnection patchReverbToMix;
    AudioConnection patchInToRectifier;
    AudioConnection patchRectifierToMix;
    AudioConnection patchInToEQ;
    AudioConnection patchEQToMix;
    AudioConnection patchInToDIST;
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

    /**
     * @brief Set chorus parameters
     * @param numVoices Number of chorus voices (1+)
     */
    void setChorusVoices(int numVoices);
    /**
     * @brief Set chorus delay buffer active length (samples)
     * @param delaySamples Size in samples to use from the preallocated buffer.
     *        Must be >= 20 and <= CHORUS_BUFFER_SIZE. Changing this at runtime
     *        will change the effective chorus amount (longer delay → more pronounced chorus).
     */
    void setChorusDelay(int delaySamples);

    /**
     * @brief Set reverb parameters
     * @param roomsize Room size (0.0 to 1.0)
     * @param damping Damping level (0.0 to 1.0)
     */
    void setReverbParams(float roomsize, float damping);

    /**
     * @brief Configure EQ (biquad) filter
     * @param stage Filter stage (0-3 for up to 4 cascaded biquads)
     * @param type Filter type: "lowpass", "highpass", "bandpass", "notch", "lowshelf", "highshelf"
     * @param frequency Center/cutoff frequency in Hz
     * @param qOrGain Q factor for bandpass/notch, gain for shelf filters
     */
    void setEQ(uint32_t stage, const char *type, float frequency, float qOrGain = 0.7071f);

    /**
     * @brief Get output mixer for patching to other audio objects
     * @return Pointer to output mixer object
     */
    AudioMixer4* getOutputMixer(void) { return &out; }

private:
    uint8_t activeEffect;  // Currently active effect index
    float32_t wetDry;      // Wet/dry mix ratio
    
    // Effect buffers
    static const int CHORUS_BUFFER_SIZE = 8820;  // ~200ms at 44.1kHz
    short chorusBuffer[CHORUS_BUFFER_SIZE];
    // Current number of chorus voices (kept so we can re-init chorus when changing delay)
    int chorusVoices;

    float distGain;
    float distBias;
    float distBiasTanh;
    float distMaxOutput;
    static const int SHAPE_LEN = 2049;
    float shape_lut[SHAPE_LEN];
};


