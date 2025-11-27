#pragma once
#include <lvgl.h>
#include <ILI9341_t3n.h>

// Pins defined here (must match your hardware)
#define TFT_CS   10
#define TFT_DC   35
#define TFT_RST  9

// The TFT object instance
extern ILI9341_t3n tft;

void display_driver_init();