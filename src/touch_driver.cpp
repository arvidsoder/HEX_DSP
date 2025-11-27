#include "touch_driver.h"
#include <Arduino.h>
#include <SPI.h>

// --- TOUCH CALIBRATION CONSTANTS (Derived from user's working test) ---
#define X_RAW_MIN 310   // Raw bounds for one axis
#define X_RAW_MAX 3800  // Raw bounds for one axis
#define Y_RAW_MIN 310   // Raw bounds for the other axis
#define Y_RAW_MAX 3800  // Raw bounds for the other axis
// --- SPI Speed (2 MHz for XPT2046) ---
#define XPT2046_SPI_SPEED 2000000 

// Define the XPT2046 object using the pins from the header
XPT2046_Touchscreen ts(T_CS, T_IRQ);

// Define the exported input device handle
lv_indev_t *touch_indev = NULL; 

// *** GLOBAL FLAG FOR EXTERNAL DEBUGGING ***
static bool g_touch_state_active = false;

// LVGL Touch Read Callback
static void my_touch_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    TS_Point p;
    
    // ** CRITICAL FIX: Ensure touch access is wrapped in a dedicated SPI transaction **
    // This isolates the slower XPT2046 chip from the faster display transactions.
    
    // Step 1: Check for touch. This is a SPI operation and must be isolated.
    // Use SPISettings optimized for the XPT2046 (low speed, SPI_MODE0)
    SPI.beginTransaction(SPISettings(XPT2046_SPI_SPEED, MSBFIRST, SPI_MODE0));
    bool is_touching = ts.touched();
    SPI.endTransaction();
    
    if (is_touching) {
        
        // Step 2: Read touch point. This is also a SPI operation.
        SPI.beginTransaction(SPISettings(XPT2046_SPI_SPEED, MSBFIRST, SPI_MODE0));
        p = ts.getPoint(); 
        SPI.endTransaction();
        
        // Check Z-pressure for a valid touch
        if (p.z < 4000) { 
            data->state = LV_INDEV_STATE_PRESSED;

            // --- CALIBRATION AND MAPPING (SWAPPED AXES) ---
            
            // Map Raw Y (p.y) to Display X (0-320)
            int32_t x_mapped = map(p.x, X_RAW_MIN, X_RAW_MAX, 0, 320);
            
            // Map Raw X (p.x) to Display Y (0-240)
            int32_t y_mapped = map(p.y, Y_RAW_MIN, Y_RAW_MAX, 0, 240);
            
            // Invert Y axis to match display orientation

            // Constrain values and assign to LVGL data structure
            data->point.x = constrain(x_mapped, 0, 319);
            data->point.y = constrain(y_mapped, 0, 239);
            
            g_touch_state_active = true;
            
        } else {
            // Pressure indicates release or invalid touch
            data->state = LV_INDEV_STATE_RELEASED;
            g_touch_state_active = false;
        }
        
    } else {
        // Not touching the screen
        data->state = LV_INDEV_STATE_RELEASED;
        g_touch_state_active = false;
    }
}

void touch_driver_init()
{
    // ** CRITICAL FIX: Ensure T_IRQ pin is configured **
    pinMode(T_IRQ, INPUT);
    
    // Initialize the touch hardware, explicitly passing the default SPI object
    // This allows the touch library to manage the SPI bus settings
    ts.begin(SPI);
    
    // Initialize the LVGL input device structure
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;

    // Register and store the handle
    touch_indev = lv_indev_drv_register(&indev_drv);
}

// Implementation of the safe touch check function
bool is_touch_active() {
    // Simply return the state of the flag updated by the LVGL read callback
    return g_touch_state_active; 
}