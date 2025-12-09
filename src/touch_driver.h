#pragma once
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>

// Pins defined here (must match your hardware)
#define T_CS  34
#define T_IRQ 41

extern XPT2046_Touchscreen ts;

void touch_driver_init();

// Export the LVGL input device handle for external polling (e.g., in main.cpp)
extern lv_indev_t *touch_indev;

// New function to safely check touch state from main.cpp
// This is now a simple external flag reader.
bool is_touch_active();