#include "ui.h"
#include "ui_helpers.h"
#include "../../src/audio_driver.h"

// Deklarera externa variabler från ui_helpers.c
extern int active_slot_index;
extern int current_slot_effects[8];
extern const char* effect_titles[8];
extern const char* param_names[8][8];

// Funktionsdeklarationer från ui_helpers.c
void SetSlotImage(int slotIndex, int effectID);
int GetEffectID(const char* name);
void SetActiveButtonState(lv_obj_t * btn, int index);
void SetSliderVisibility(lv_obj_t* slider, lv_obj_t* label, const char* name);
void ApplyPresetToSlot(int slot, const char* name, int p1, int p2, int p3, int p4);

// --- PRESETS ---

void ClearSlot(int slot) { ApplyPresetToSlot(slot, "None", 0, 0, 0, 0); }

void ui_event_BtnPreset1(lv_event_t * e) { 
    ApplyPresetToSlot(0, "Compressor", 60, 50, 20, 80);
    ApplyPresetToSlot(1, "Delay", 55, 40, 50, 30);
    ApplyPresetToSlot(2, "Reverb", 70, 50, 0, 40);
    ClearSlot(3); ClearSlot(4); ClearSlot(5); ClearSlot(6); ClearSlot(7);
}

void ui_event_BtnPreset2(lv_event_t * e) { 
    ApplyPresetToSlot(0, "Distortion", 75, 60, 80, 0);
    ApplyPresetToSlot(1, "EQ", 50, 70, 60, 80);
    ApplyPresetToSlot(2, "Delay", 20, 20, 50, 25);
    ApplyPresetToSlot(3, "Amplifier", 80, 60, 70, 90);
    ClearSlot(4); ClearSlot(5); ClearSlot(6); ClearSlot(7);
}

void ui_event_BtnPreset3(lv_event_t * e) { 
    ApplyPresetToSlot(0, "Compressor", 50, 50, 50, 90);
    ApplyPresetToSlot(1, "Distortion", 35, 60, 70, 0);
    ApplyPresetToSlot(2, "Amplifier", 60, 50, 60, 85);
    ApplyPresetToSlot(3, "Reverb", 20, 80, 0, 20);
    ClearSlot(4); ClearSlot(5); ClearSlot(6); ClearSlot(7);
}

// --- EVENTS ---

// Checkbox Mono event handler
// Toggles the Boolean Switch param.
// For Standard effects: Param 5.
// For EQ: Param 8 (to avoid conflict with Band 6 or Mix/Level).
void ui_event_CheckboxMono(lv_event_t * e) {
    if (active_slot_index < 0) return;
    lv_obj_t * obj = lv_event_get_target(e);
    int is_checked = lv_obj_has_state(obj, LV_STATE_CHECKED) ? 1 : 0;
    
    int effectID = current_slot_effects[active_slot_index];
    //if (effectID == 7) {
    //    dsp_set_effect_param(active_slot_index, 8, is_checked);
    //} else {
    //    dsp_set_effect_param(active_slot_index, 5, is_checked);
    //}
}

// Hanterar alla sliders
void slider_value_changed(lv_event_t * e){
    lv_obj_t * slider = lv_event_get_target(e);
    int val = 0;
    
    if(lv_obj_check_type(slider, &lv_slider_class)) val = lv_slider_get_value(slider);
    else if(lv_obj_check_type(slider, &lv_arc_class)) val = lv_arc_get_value(slider);

    if (slider == ui_ArcInputGain) {
        //dsp_set_input_gain(val);
        if(ui_LabelInputValue) lv_label_set_text_fmt(ui_LabelInputValue, "%d", val);
    } 
    else if (slider == ui_SliderOutputLevel) {
        setMasterOutputLevel((float)val / 100.0f);
        if(ui_ValueOutputLevel) lv_label_set_text_fmt(ui_ValueOutputLevel, "%d %%", val);
    }
    // Effekt-parametrar (bara om vi har en vald slot)
    else if (active_slot_index >= 0) {
        int effectID = current_slot_effects[active_slot_index];

        if (slider == ui_SliderParam1) setStageParameter(active_slot_index, effectID, 0, val);
        else if (slider == ui_SliderParam2) setStageParameter(active_slot_index, effectID, 1, val);
        else if (slider == ui_SliderParam3) setStageParameter(active_slot_index, effectID, 2, val);
        else if (slider == ui_SliderParam4) setStageParameter(active_slot_index, effectID, 3, val);
        else if (slider == ui_SliderParam5) {
            // Slider 5 (Index 4) -> Only used for EQ (Band 5)
            if (effectID == 7) setStageParameter(active_slot_index, effectID, 4, val);
        }
        else if (slider == ui_SliderParam6) {
            // Slider 6 (Index 5) -> Only used for EQ (Band 6)
            if (effectID == 7) setStageParameter(active_slot_index, effectID, 5, val);
        }
        else if (slider == ui_SliderParam7) {
            // Slider 7 (Index 6) -> Mix (for ALL effects)
            setStageParameter(active_slot_index, effectID, 6, val);
        }
        else if (slider == ui_SliderParam8) {
            // Slider 8 (Index 7) -> Mono Level (for ALL effects)
            setStageParameter(active_slot_index, effectID, 7, val);
        }
    }
}

// Identifierar vilken slot-knapp som trycktes
void SelectActiveSlot(lv_event_t * e){
    lv_obj_t * btn = lv_event_get_target(e);
    
    if (btn == ui_BtnReverbIcon)      SetActiveButtonState(btn, 0); 
    else if (btn == ui_BtnDelayIcon)  SetActiveButtonState(btn, 1); 
    else if (btn == ui_BtnDistortionIcon) SetActiveButtonState(btn, 2); 
    else if (btn == ui_BtnCompressorIcon) SetActiveButtonState(btn, 3); 
    else if (btn == ui_BtnChorusIcon) SetActiveButtonState(btn, 4); 
    else if (btn == ui_BtnCabIcon)    SetActiveButtonState(btn, 5); 
    else if (btn == ui_BtnAmpIcon)    SetActiveButtonState(btn, 6); 
    else if (btn == ui_BtnEQIcon)     SetActiveButtonState(btn, 7); 
}

// Öppnar parameter-sidan
void OnBtnEditParamsClicked(lv_event_t * e){
    if (active_slot_index < 0) return; 

    int effectID = current_slot_effects[active_slot_index];
    if(ui_LabelEditTitle) lv_label_set_text(ui_LabelEditTitle, effect_titles[effectID]);

    lv_obj_t* sliders[8] = {ui_SliderParam1, ui_SliderParam2, ui_SliderParam3, ui_SliderParam4, ui_SliderParam5, ui_SliderParam6, ui_SliderParam7, ui_SliderParam8};
    // Include ui_LabelParam8 in the array
    lv_obj_t* labels[8] = {ui_LabelParam1, ui_LabelParam2, ui_LabelParam3, ui_LabelParam4, ui_LabelParam5, ui_LabelParam6, ui_LabelParam7, ui_LabelParam8};

    for(int i=0; i<8; i++) {
        const char* pName = param_names[effectID][i];
        
        // FORCE "Mono Vol" for the 8th parameter (Index 7) for ALL effects
        if (i == 7) {
            pName = "Mono Vol";
        }

        SetSliderVisibility(sliders[i], labels[i], pName);
    }
    
    // Checkbox state should ideally be read from DSP here, 
    // but assuming UI state persistence or defaults for now.
    
    _ui_screen_change(&ui_PanelEditHeader, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_PanelEditHeader_screen_init);
}

void OnBtnReplaceClicked(lv_event_t * e){
    if (active_slot_index < 0) return;
    _ui_screen_change(&ui_ScreenEffectSelector, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_ScreenEffectSelector_screen_init);
}

void UpdateSlotEffect(lv_event_t * e){
    char buf[32];
    if(ui_RollerEffectSelect) lv_roller_get_selected_str(ui_RollerEffectSelect, buf, sizeof(buf));
    else return; 

    if (strcmp(buf, "Cancel") == 0 || strlen(buf) == 0) {
         _ui_screen_change(&ui_DifferentEffects, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_DifferentEffects_screen_init);
         return;
    }

    lv_obj_t* labels[8] = {ui_LabelReverb, ui_LabelDelay, ui_LabelDistortion, ui_LabelComp, ui_LabelChorus, ui_LabelCABIR, ui_LabelAmp, ui_LabelEQ};
    
    if (active_slot_index >= 0 && active_slot_index < 8 && labels[active_slot_index] != NULL) {
        lv_label_set_text(labels[active_slot_index], buf);
    }
    
    int newEffectID = GetEffectID(buf);
    if (active_slot_index >= 0) {
        SetSlotImage(active_slot_index, newEffectID);
        current_slot_effects[active_slot_index] = newEffectID;
    }

    setStageEffect(active_slot_index, newEffectID);

     // Reset parameters to defaults when changing effect
    
    _ui_screen_change(&ui_DifferentEffects, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_DifferentEffects_screen_init);
}