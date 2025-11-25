#include <lvgl.h>
#include <ILI9341_t3n.h>

// --- YOUR PINS ---
#define TFT_CS   10
#define TFT_DC   35

// ILI9341_t3n object (no reset pin)
ILI9341_t3n tft = ILI9341_t3n(TFT_CS, TFT_DC);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    int32_t w = (area->x2 - area->x1 + 1);
    int32_t h = (area->y2 - area->y1 + 1);

    // Use t3n fast write
    tft.writeRect(area->x1, area->y1, w, h, (uint16_t*)color_p);

    lv_disp_flush_ready(disp);
}

void display_driver_init() {
    tft.begin();
    tft.setRotation(1);   // Landscape

    // Optional but recommended for speed
    tft.useFrameBuffer(true);

    // LVGL draw buffer (32-bit per pixel * 320 * 20 strip)
    buf1 = (lv_color_t*)malloc(sizeof(lv_color_t) * 320 * 20);

    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 320 * 20);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;

    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);
}
