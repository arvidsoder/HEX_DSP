#pragma once
#include <lvgl.h>
#include <ILI9341_t3n.h>

// Pins defined here (must match your hardware)
#define TFT_CS   0
#define TFT_DC   2
#define TFT_RST  3
#define TFT_MOSI 26
#define TFT_SCLK 27
#define TFT_MISO 1

// The TFT object instance
extern ILI9341_t3n tft;

void display_driver_init();