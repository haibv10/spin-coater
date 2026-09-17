#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include <stdint.h>
#include "screen_analog.h"
#include "screen_digital.h"
#include "screen_ramp.h" 


#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        APP_MODE_NONE = 0,
        APP_MODE_ANALOG,
        APP_MODE_DIGITAL,
        APP_MODE_RAMP,
        APP_MODE_SETTING,
    } AppMode_t;

// Khởi tạo control
void App_Control_Init(void);

// Chọn mode (gọi khi nhấn nút trong menu)
void App_Control_SelectMode(AppMode_t mode);

// Lấy mode hiện tại
AppMode_t App_Control_GetCurrentMode(void);

#ifdef __cplusplus
}
#endif

#endif