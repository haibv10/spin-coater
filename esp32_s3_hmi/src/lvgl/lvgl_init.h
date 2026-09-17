#ifndef _LVGL_INIT_H_
#define _LVGL_INIT_H_

#include <lvgl.h>
#include <Arduino.h>
#include "pincfg.h"
#include "dispcfg.h"
#include "AXS15231B_touch.h"
#include <Arduino_GFX_Library.h>

void lvgl_init();
uint32_t millis_cb(void);
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

void lvgl_loop(void);

#endif