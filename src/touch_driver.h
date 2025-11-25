#pragma once
#include <XPT2046_Touchscreen.h>

// Expose your touch object so other files can access it
extern XPT2046_Touchscreen ts;

// Initialization function (implemented in touch_driver.cpp)
void touch_driver_init();
