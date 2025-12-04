#include <Audio.h>

#define CHORUS_FX_INDEX      1
#define REVERB_FX_INDEX      2
#define RECTIFIER_FX_INDEX   3
#define EQ_FX_INDEX          4
#define FIR_FX_INDEX         5

class EffectMixer8 {
public:
    AudioAmplifier in;
    AudioMixer4 mixerA;
    AudioMixer4 mixerB;
    AudioMixer4 out; 
    AudioEffectChorus chorusFx;         // 1
    AudioEffectFreeverb reverbFx;       // 2
    AudioEffectRectifier rectifierFx;   // 3
    AudioFilterBiquad eqFx;             // 4
    AudioFilterFIR firFx;               // 5 

    AudioConnection patchInToMix{in, 0, mixerA, 0};
    AudioConnection patchInToChorus{in, 0, chorusFx, 0};
    AudioConnection patchChorusToMix{chorusFx, 0, mixerA, CHORUS_FX_INDEX};
    AudioConnection patchInToReverb{in, 0, reverbFx, 0};
    AudioConnection patchReverbToMix{reverbFx, 0, mixerA, REVERB_FX_INDEX};
    AudioConnection patchInToRectifier{in, 0, rectifierFx, 0};
    AudioConnection patchRectifierToMix{rectifierFx, 0, mixerA, RECTIFIER_FX_INDEX};
    AudioConnection patchInToEQ{in, 0, eqFx, 0};
    AudioConnection patchEQToMix{eqFx, 0, mixerB, EQ_FX_INDEX-4};
    AudioConnection patchInToFIR{in, 0, firFx, 0};
    AudioConnection patchFIRToMix{firFx, 0, mixerB, FIR_FX_INDEX-4};
    AudioConnection patchMixAToOut0{mixerA, 0, out, 0};
    AudioConnection patchMixBToOut1{mixerB, 0, out, 1};
    


    EffectMixer8(void) {
        for (int i = 0; i <4; i++) {
            mixerA.gain(i, 1.0f);
            mixerB.gain(i, 1.0f);
        }
        out.gain(0, 1.0f);
        out.gain(1, 1.0f);
        in.gain(1.0f);
        // Default effect settings
        reverbFx.roomsize(0.5f);
        reverbFx.damping(0.5f);
        
        chorusFx.voices(2);
        
        eqFx.setLowpass(0, 1000.0f, 0.7071f);
        firFx.begin(nullptr, 0); // bypass
        }
    
        void gain(uint8_t channel, float level) {
        if (channel < 4)
            mixerA.gain(channel, level);
        else
            mixerB.gain(channel - 4, level);
    }
    void setActiveEffect(uint8_t effectIndex) {
        switch (effectIndex) {
            case CHORUS_FX_INDEX: // Chorus
                chorusFx.enable();
                reverbFx.disable();
                rectifierFx.disable();
                eqFx.disable();
                firFx.disable();
                break;
            case REVERB_FX_INDEX: // Reverb
                chorusFx.disable();
                reverbFx.enable();
                rectifierFx.disable();
                eqFx.disable();
                firFx.disable();
                break;
            case RECTIFIER_FX_INDEX: // Rectifier
                chorusFx.disable();
                reverbFx.disable();
                rectifierFx.enable();
                eqFx.disable();
                firFx.disable();
                break;
            case EQ_FX_INDEX: // EQ
                chorusFx.disable();
                reverbFx.disable();
                rectifierFx.disable();
                eqFx.enable();
                firFx.disable();
                break;
            case FIR_FX_INDEX: // FIR
                chorusFx.disable();
                reverbFx.disable();
                rectifierFx.disable();
                eqFx.disable();
                firFx.enable();
                break;
            default:
                chorusFx.disable();
                reverbFx.disable();
                rectifierFx.disable();
                eqFx.disable();
                firFx.disable();
                break;
        }
    }
};




