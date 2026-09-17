#include "app_control.h"
#include "frame_io.h"

static AppMode_t current_mode = APP_MODE_NONE;

void App_Control_Init(void)
{
    current_mode = APP_MODE_NONE;
}

void App_Control_SelectMode(AppMode_t mode)
{
    current_mode = mode;

    switch (current_mode)
    {
    case APP_MODE_ANALOG:
        FrameIO_SendData_U8U16(TYPE_MSG_SET_ANALOG, 0);
        break;
    case APP_MODE_DIGITAL:
        FrameIO_SendData_U8U16(TYPE_MSG_SET_DIGITAL, 0);
        break;
    case APP_MODE_RAMP:
        FrameIO_SendData_U8U16(TYPE_MSG_SET_RAMP, 0);
        break;
    default:
        break;
    }
}

AppMode_t App_Control_GetCurrentMode(void)
{
    return current_mode;
}

