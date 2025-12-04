#include "display_driver.h"
#include <SPI.h> // Include SPI for setClock

// Define the TFT object using the pins from the header
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC, TFT_RST);

// LVGL needs two buffers for smooth, tearing-free drawing (about 1/12th screen area)
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[320 * 20];   
static lv_color_t buf2[320 * 20];

// LVGL Display Flush Callback
static void my_disp_flush(lv_disp_drv_t *disp,
                          const lv_area_t *area,
                          lv_color_t *color_p)
{
    uint16_t w = area->x2 - area->x1 + 1;
    uint16_t h = area->y2 - area->y1 + 1;

    // Use the ILI9341_t3n writeRect command for fast DMA transfer
    tft.writeRect(area->x1, area->y1, w, h, (uint16_t*)color_p);

    // Tell LVGL that the flush is complete
    lv_disp_flush_ready(disp);
}

void display_driver_init()
{
    // Initialize the display hardware, passing the desired SPI clock frequency (16 MHz)
    // The begin() function in ILI9341_t3n accepts the clock speed as an optional argument.
    // If 16MHz works, you can try higher values later (e.g., 30000000 for 30MHz)
    tft.begin(16000000);
    
    // tft.setClock(16000000); // <-- REMOVED: Caused error "no member named 'setClock'"
    
    tft.setRotation(1);      // Set to Landscape (320x240)
    
    // We explicitly disable the library's internal frame buffer, as LVGL handles buffering.
    tft.useFrameBuffer(false);

    // Initialize the display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2,
                          sizeof(buf1) / sizeof(lv_color_t));

    // Initialize the LVGL display driver structure
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;

    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;

    // Register the driver
    lv_disp_drv_register(&disp_drv);
}