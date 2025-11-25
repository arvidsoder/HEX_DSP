#include <lvgl.h>
#include <XPT2046_Touchscreen.h>

// --- YOUR PINS ---
#define T_CS   34
#define T_IRQ  41

XPT2046_Touchscreen ts(T_CS, T_IRQ);

void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (!ts.touched()) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    TS_Point p = ts.getPoint();

    // Rotation compensation (ILI9341 rotation = 1)
    int16_t x = p.y;
    int16_t y = 240 - p.x;

    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
}

void touch_driver_init() {
    ts.begin();

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;

    lv_indev_drv_register(&indev_drv);
}
