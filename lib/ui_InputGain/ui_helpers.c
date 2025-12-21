// ui_InputGain/ui_helpers.c

#include "ui.h"
#include "ui_helpers.h"
#include "../../src/audio_driver.h"
#include <stdio.h>
#include <string.h>

// ... (Behåll standard SquareLine-hjälpfunktioner här om de inte ändrats) ...

void _ui_bar_set_property(lv_obj_t * target, int id, int val) {
    if(id == _UI_BAR_PROPERTY_VALUE_WITH_ANIM) lv_bar_set_value(target, val, LV_ANIM_ON);
    if(id == _UI_BAR_PROPERTY_VALUE) lv_bar_set_value(target, val, LV_ANIM_OFF);
}

void _ui_basic_set_property(lv_obj_t * target, int id, int val) {
    if(id == _UI_BASIC_PROPERTY_POSITION_X) lv_obj_set_x(target, val);
    if(id == _UI_BASIC_PROPERTY_POSITION_Y) lv_obj_set_y(target, val);
    if(id == _UI_BASIC_PROPERTY_WIDTH) lv_obj_set_width(target, val);
    if(id == _UI_BASIC_PROPERTY_HEIGHT) lv_obj_set_height(target, val);
}

void _ui_dropdown_set_property(lv_obj_t * target, int id, int val) {
    if(id == _UI_DROPDOWN_PROPERTY_SELECTED) lv_dropdown_set_selected(target, val);
}

void _ui_image_set_property(lv_obj_t * target, int id, uint8_t * val) {
    if(id == _UI_IMAGE_PROPERTY_IMAGE) lv_img_set_src(target, val);
}

void _ui_label_set_property(lv_obj_t * target, int id, const char * val) {
    if(id == _UI_LABEL_PROPERTY_TEXT) lv_label_set_text(target, val);
}

void _ui_roller_set_property(lv_obj_t * target, int id, int val) {
    if(id == _UI_ROLLER_PROPERTY_SELECTED_WITH_ANIM) lv_roller_set_selected(target, val, LV_ANIM_ON);
    if(id == _UI_ROLLER_PROPERTY_SELECTED) lv_roller_set_selected(target, val, LV_ANIM_OFF);
}

void _ui_slider_set_property(lv_obj_t * target, int id, int val) {
    if(id == _UI_SLIDER_PROPERTY_VALUE_WITH_ANIM) lv_slider_set_value(target, val, LV_ANIM_ON);
    if(id == _UI_SLIDER_PROPERTY_VALUE) lv_slider_set_value(target, val, LV_ANIM_OFF);
}

void _ui_screen_change(lv_obj_t ** target, lv_scr_load_anim_t fademode, int spd, int delay, void (*target_init)(void)) {
    if(*target == NULL) target_init();
    lv_scr_load_anim(*target, fademode, spd, delay, false);
}

void _ui_arc_increment(lv_obj_t * target, int val) {
    int old = lv_arc_get_value(target);
    lv_arc_set_value(target, old + val);
    lv_event_send(target, LV_EVENT_VALUE_CHANGED, 0);
}

void _ui_bar_increment(lv_obj_t * target, int val, int anm) {
    int old = lv_bar_get_value(target);
    lv_bar_set_value(target, old + val, anm);
}

void _ui_slider_increment(lv_obj_t * target, int val, int anm) {
    int old = lv_slider_get_value(target);
    lv_slider_set_value(target, old + val, anm);
    lv_event_send(target, LV_EVENT_VALUE_CHANGED, 0);
}

void _ui_keyboard_set_target(lv_obj_t * keyboard, lv_obj_t * textarea) {
    lv_keyboard_set_textarea(keyboard, textarea);
}

void _ui_flag_modify(lv_obj_t * target, int32_t flag, int value) {
    if(value == _UI_MODIFY_FLAG_TOGGLE) {
        if(lv_obj_has_flag(target, flag)) lv_obj_clear_flag(target, flag);
        else lv_obj_add_flag(target, flag);
    }
    else if(value == _UI_MODIFY_FLAG_ADD) lv_obj_add_flag(target, flag);
    else lv_obj_clear_flag(target, flag);
}

void _ui_state_modify(lv_obj_t * target, int32_t state, int value) {
    if(value == _UI_MODIFY_STATE_TOGGLE) {
        if(lv_obj_has_state(target, state)) lv_obj_clear_state(target, state);
        else lv_obj_add_state(target, state);
    }
    else if(value == _UI_MODIFY_STATE_ADD) lv_obj_add_state(target, state);
    else lv_obj_clear_state(target, state);
}

void _ui_textarea_move_cursor(lv_obj_t * target, int val) {
    if(val == UI_MOVE_CURSOR_UP) lv_textarea_cursor_up(target);
    if(val == UI_MOVE_CURSOR_RIGHT) lv_textarea_cursor_right(target);
    if(val == UI_MOVE_CURSOR_DOWN) lv_textarea_cursor_down(target);
    if(val == UI_MOVE_CURSOR_LEFT) lv_textarea_cursor_left(target);
    lv_obj_add_state(target, LV_STATE_FOCUSED);
}

typedef void (*screen_destroy_cb_t)(void);
void scr_unloaded_delete_cb(lv_event_t * e) {
    screen_destroy_cb_t destroy_cb = (screen_destroy_cb_t)lv_event_get_user_data(e);
    if(destroy_cb) destroy_cb();
}

void _ui_opacity_set(lv_obj_t * target, int val) {
    lv_obj_set_style_opa(target, val, 0);
}

void _ui_anim_callback_free_user_data(lv_anim_t * a) {
    lv_mem_free(a->user_data);
    a->user_data = NULL;
}

void _ui_anim_callback_set_x(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    lv_obj_set_x(usr->target, v);
}

void _ui_anim_callback_set_y(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    lv_obj_set_y(usr->target, v);
}

void _ui_anim_callback_set_width(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    lv_obj_set_width(usr->target, v);
}

void _ui_anim_callback_set_height(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    lv_obj_set_height(usr->target, v);
}

void _ui_anim_callback_set_opacity(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    lv_obj_set_style_opa(usr->target, v, 0);
}

void _ui_anim_callback_set_image_zoom(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    lv_img_set_zoom(usr->target, v);
}

void _ui_anim_callback_set_image_angle(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    lv_img_set_angle(usr->target, v);
}

void _ui_anim_callback_set_image_frame(lv_anim_t * a, int32_t v) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    usr->val = v;
    if(v < 0) v = 0;
    if(v >= usr->imgset_size) v = usr->imgset_size - 1;
    lv_img_set_src(usr->target, usr->imgset[v]);
}

int32_t _ui_anim_callback_get_x(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return lv_obj_get_x_aligned(usr->target);
}

int32_t _ui_anim_callback_get_y(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return lv_obj_get_y_aligned(usr->target);
}

int32_t _ui_anim_callback_get_width(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return lv_obj_get_width(usr->target);
}

int32_t _ui_anim_callback_get_height(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return lv_obj_get_height(usr->target);
}

int32_t _ui_anim_callback_get_opacity(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return lv_obj_get_style_opa(usr->target, 0);
}

int32_t _ui_anim_callback_get_image_zoom(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return lv_img_get_zoom(usr->target);
}

int32_t _ui_anim_callback_get_image_angle(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return lv_img_get_angle(usr->target);
}

int32_t _ui_anim_callback_get_image_frame(lv_anim_t * a) {
    ui_anim_user_data_t * usr = (ui_anim_user_data_t *)a->user_data;
    return usr->val;
}

void _ui_arc_set_text_value(lv_obj_t * trg, lv_obj_t * src, const char * prefix, const char * postfix) {
    char buf[_UI_TEMPORARY_STRING_BUFFER_SIZE];
    lv_snprintf(buf, sizeof(buf), "%s%d%s", prefix, (int)lv_arc_get_value(src), postfix);
    lv_label_set_text(trg, buf);
}

void _ui_slider_set_text_value(lv_obj_t * trg, lv_obj_t * src, const char * prefix, const char * postfix) {
    char buf[_UI_TEMPORARY_STRING_BUFFER_SIZE];
    lv_snprintf(buf, sizeof(buf), "%s%d%s", prefix, (int)lv_slider_get_value(src), postfix);
    lv_label_set_text(trg, buf);
}

void _ui_checked_set_text_value(lv_obj_t * trg, lv_obj_t * src, const char * txt_on, const char * txt_off) {
    if(lv_obj_has_state(src, LV_STATE_CHECKED)) lv_label_set_text(trg, txt_on);
    else lv_label_set_text(trg, txt_off);
}

void _ui_spinbox_step(lv_obj_t * target, int val) {
    if(val > 0) lv_spinbox_increment(target);
    else lv_spinbox_decrement(target);
    lv_event_send(target, LV_EVENT_VALUE_CHANGED, 0);
}

void _ui_switch_theme(int val) {
#ifdef UI_THEME_ACTIVE
    ui_theme_set(val);
#endif
}

// --- CUSTOM LOGIC ---

int active_slot_index = -1;
lv_obj_t * active_slot_btn = NULL;
int current_slot_effects[8] = {0}; 

// UPDATED MAPPING:
// Slider 7 (Index 6) = Mix
// Slider 8 (Index 7) = Mono Level (Controlled via slider next to Mono checkbox)
// The text for index 7 "Mono Lvl" might not appear if the Label object is missing in UI, but the slider will work.
const char* param_names[8][8] = {
    {"-", "-", "-", "-", "-", "-", "-", "-"},                   // 0: None
    {"Room", "Damp", "Tone", "PreDel", "-", "-", "Mix", "Mono Lvl"},    // 1: Reverb
    {"Time", "F.Back", "Tone", "Rate", "-", "-", "Mix", "Mono Lvl"},    // 2: Delay
    {"Drive", "Tone", "Crunch", "Edge", "-", "-", "Mix", "Mono Lvl"},   // 3: Distortion
    {"Thresh", "Ratio", "Attack", "Releas", "-", "-", "Mix", "Mono Lvl"}, // 4: Compressor
    {"Speed", "Depth", "Tone", "Delay", "-", "-", "Mix", "Mono Lvl"},   // 5: Chorus
    {"Gain", "Bass", "Mid", "Treble", "-", "-", "Mix", "Mono Lvl"},     // 6: Amp
    {"100Hz", "200Hz", "400Hz", "800Hz", "1.5kHz", "3.2kHz", "Mix", "Mono Lvl"} // 7: EQ
};

const char* effect_titles[8] = {
    "Empty Slot", "Reverb", "Delay", "Distortion", 
    "Compressor", "Chorus", "Amplifier", "6-Band EQ"
};

const void* effect_images[8] = {
    &ui_img_add_60dp_ffffff_fill0_wght400_grad0_opsz48_png,        
    &ui_img_music_note_60dp_e3e3e3_fill0_wght400_grad0_opsz48_png, 
    &ui_img_timer_arrow_up_60dp_e3e3e3_fill0_wght400_grad0_opsz48_png, 
    &ui_img_earthquake_60dp_e3e3e3_fill0_wght400_grad0_opsz48_png, 
    &ui_img_cadence_60dp_e3e3e3_fill0_wght400_grad0_opsz48_png,    
    &ui_img_wifi_channel_60dp_e3e3e3_fill0_wght400_grad0_opsz48_png, 
    &ui_img_speaker_60dp_e3e3e3_fill0_wght400_grad0_opsz48_png,    
    &ui_img_equalizer_60dp_e3e3e3_fill0_wght400_grad0_opsz48_png   
};

int GetEffectID(const char* name) {
    if (strcmp(name, "Reverb") == 0) return 1;
    if (strcmp(name, "Delay") == 0) return 2;
    if (strcmp(name, "Distortion") == 0) return 3;
    if (strcmp(name, "Compressor") == 0) return 4;
    if (strcmp(name, "Comp") == 0) return 4;
    if (strcmp(name, "Chorus") == 0) return 5;
    if (strcmp(name, "Amplifier") == 0) return 6;
    if (strcmp(name, "Amp") == 0) return 6;
    if (strcmp(name, "EQ") == 0) return 7;
    return 0; 
}

void SetSlotImage(int slotIndex, int effectID) {
    if (slotIndex < 0 || slotIndex > 7) return;
    if (effectID < 0 || effectID > 7) return;

    lv_obj_t* btn = NULL;
    switch(slotIndex) {
        case 0: btn = ui_BtnReverbIcon; break;
        case 1: btn = ui_BtnDelayIcon; break; 
        case 2: btn = ui_BtnDistortionIcon; break; 
        case 3: btn = ui_BtnCompressorIcon; break; 
        case 4: btn = ui_BtnChorusIcon; break; 
        case 5: btn = ui_BtnCabIcon; break;    
        case 6: btn = ui_BtnAmpIcon; break;    
        case 7: btn = ui_BtnEQIcon; break;     
    }
    
    if(btn) {
       lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_RELEASED, NULL, effect_images[effectID], NULL);
       lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_PRESSED, NULL, effect_images[effectID], NULL);
       lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, effect_images[effectID], NULL);
       lv_imgbtn_set_src(btn, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, effect_images[effectID], NULL);
    }
}

void SetActiveButtonState(lv_obj_t * btn, int index) {
    if (active_slot_btn != NULL && active_slot_btn != btn) {
        lv_obj_clear_state(active_slot_btn, LV_STATE_CHECKED);
    }
    active_slot_btn = btn;
    lv_obj_add_state(active_slot_btn, LV_STATE_CHECKED);
    active_slot_index = index;
}

void SetSliderVisibility(lv_obj_t* slider, lv_obj_t* label, const char* name) {
    if (slider == NULL) return; // Label is allowed to be NULL (e.g. for Slider 8)
    
    if (strcmp(name, "-") == 0) {
        lv_obj_add_flag(slider, LV_OBJ_FLAG_HIDDEN);
        if (label) lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(slider, LV_OBJ_FLAG_HIDDEN);
        if (label) {
            lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(label, name);
        }
    }
}

void ApplyPresetToSlot(int slot, const char* name, int p1, int p2, int p3, int p4) {
    int id = GetEffectID(name);
    current_slot_effects[slot] = id;
    
    SetSlotImage(slot, id);
    
    lv_obj_t* labels[8] = {ui_LabelReverb, ui_LabelDelay, ui_LabelDistortion, ui_LabelComp, ui_LabelChorus, ui_LabelCABIR, ui_LabelAmp, ui_LabelEQ};
    if(labels[slot]) lv_label_set_text(labels[slot], (id == 0) ? "-" : name);

    setStageEffect(slot, id);
    if (id != 0) {
        setStageParameter(slot, id, 0, p1);
        setStageParameter(slot, id, 1, p2);
        setStageParameter(slot, id, 2, p3);
        setStageParameter(slot, id, 3, p4);
        // Defaults
        setStageParameter(slot, id, 6, 1);
        setStageParameter(slot, id, 7, 1); // Mono Level default (Param 7)
        
        // Mono Switch Reset
        if(id == 7) setStageParameter(slot, id, 8, 0); // Param 8 for EQ switch
        else setStageParameter(slot, id, 5, 0); // Param 5 for Std switch
    }
}